/*! \file PubSubNode.hpp
    \brief Containing provisions for Publish Subscribe (PubSub) Server functionality
*/

#ifndef MARLIN_PUBSUB_PUBSUBNODE_HPP
#define MARLIN_PUBSUB_PUBSUBNODE_HPP

#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/tcp/TcpOutFiber.hpp>
#include <marlin/core/fibers/DynamicFramingFiber.hpp>
#include <marlin/core/fibers/SentinelFramingFiber.hpp>
#include <marlin/core/fibers/SentinelBufferFiber.hpp>
#include <marlin/core/fibers/LengthFramingFiber.hpp>
#include <marlin/core/fibers/LengthBufferFiber.hpp>
#include <marlin/asyncio/udp/UdpTransportFactory.hpp>
#include <marlin/asyncio/tcp/TcpTransportFactory.hpp>
#include <marlin/stream/StreamTransportFactory.hpp>
#include <marlin/lpf/LpfTransportFactory.hpp>

#include <algorithm>
#include <map>
#include <string>
#include <list>
#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <random>
#include <unordered_set>
#include <tuple>

#include <secp256k1_recovery.h>
#include <cryptopp/keccak.h>
#include <spdlog/fmt/fmt.h>
#include <rapidjson/document.h>

#include "marlin/pubsub/PubSubTransportSet.hpp"
#include "marlin/pubsub/DefaultAbci.hpp"
#include "marlin/pubsub/attestation/EmptyAttester.hpp"
#include "marlin/pubsub/witness/EmptyWitnesser.hpp"


namespace std {
	/// Hash function for SocketAddress so it can be used as a key
	template <
		>
	struct hash<std::array<uint8_t, 20>>
	{
		/// Hash function for SocketAddress so it can be used as a key
		size_t operator()(std::array<uint8_t, 20> const& addr) const
		{
			return std::hash<uint64_t>()(*(uint64_t*)addr.data()) ^ std::hash<uint64_t>()(*(uint64_t*)(addr.data()+8)) ^ std::hash<uint32_t>()(*(uint32_t*)(addr.data()+16));
		}
	};
}


namespace marlin {

namespace lpf {
template<typename Delegate>
struct IsTransportEncrypted<stream::StreamTransport<
	Delegate,
	asyncio::UdpTransport
>> {
	constexpr static bool value = true;
};
}

namespace pubsub {

int status;

template<typename X>
using SentinelFramingFiberHelper = core::SentinelFramingFiber<X, '\n'>;

struct StakeRequester {
	using FiberType = core::Fabric<
		StakeRequester&,
		asyncio::TcpOutFiber,
		core::DynamicFramingFiberHelper<
			core::FabricF<SentinelFramingFiberHelper, core::SentinelBufferFiber>::type,
			core::FabricF<core::LengthFramingFiber, core::LengthBufferFiber>::type
		>::type
	>;

	std::string staking_url;
	std::string network_id;

	asyncio::Timer t;
	asyncio::Timer dns_timer;
	core::SocketAddress dst;

	void query_cb() {
		SPDLOG_DEBUG("Timer hit: {}", dst.to_string());
		if(dst == core::SocketAddress()) {
			return;
		}
		auto& fiber = *new FiberType(std::forward_as_tuple(
			*this,
			std::make_tuple(),
			std::make_tuple(std::make_tuple(
				std::make_tuple(),
				std::make_tuple()
			))
		));
		(void)fiber.template outer_call<"dial"_tag>(*this, dst);
	}

	void dns_cb() {
		SPDLOG_DEBUG("DNS timer hit: {}", dst.to_string());
		uv_getaddrinfo_t* req = new uv_getaddrinfo_t();
		req->data = this;
		auto res = uv_getaddrinfo(uv_default_loop(), req, [](uv_getaddrinfo_t* req, int, addrinfo* res) {
			auto& sr = *(StakeRequester*)req->data;
			if(res != nullptr) {
				auto addr = core::SocketAddress(*res->ai_addr);

				SPDLOG_DEBUG("DNS result: {}", addr.to_string());

				sr.dst = addr;
			}

			uv_freeaddrinfo(res);
			delete req;
		}, "graph.marlin.pro", "http", nullptr);
		if(res != 0) {
			SPDLOG_ERROR("DNS lookup error: {}", res);
		}
	}

	size_t state = 0;
	size_t length = 0;

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "did_recv"_tag) {
			return did_recv(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_dial"_tag) {
			return did_dial(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_send"_tag) {
			return did_send(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_close"_tag) {
			return did_close(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_recv"_tag) {
			return did_recv(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_recv_skip_stream"_tag) {
			return did_recv_skip_stream(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_recv_flush_stream"_tag) {
			return did_recv_flush_stream(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_recv_flush_conf"_tag) {
			return did_recv_flush_conf(std::forward<decltype(args)>(args)...);
		} else {
			static_assert(tag < 0);
		}
	}

private:
	int did_recv(auto&& fiber, core::Buffer&& buf, core::SocketAddress addr [[maybe_unused]]) {
		SPDLOG_DEBUG("StakeRequester: Did recv: {} bytes from {}: {}", buf.size(), addr.to_string(), std::string((char*)buf.data(), buf.size()));

		if(state == 0) {
			// headers
			if(buf.size() == 2) {
				// empty
				if(length == 0) {
					// still do not have length
					fiber.template inner_call<"close"_tag>(*this);
					return -1;
				}

				// go to next phase
				state = 1;
				fiber.template inner_call<"transform"_tag>(*this, std::integral_constant<size_t, 1>{}, std::make_tuple(), std::make_tuple());
				fiber.template inner_call<"reset"_tag>(*this, length);
				return 0;
			} else if(std::memcmp(buf.data(), u8"content-length: ", 16) == 0) {
				// parse length
				auto length_str = std::string((char*)buf.data()+16, buf.size() - 18);
				length = std::stol(length_str);
			}
			fiber.template inner_call<"reset"_tag>(*this, 1000);
		} else {
			// body
			SPDLOG_DEBUG("{}", std::string((char*)buf.data(), buf.size()));
			state = 0;
			length = 0;
			{
			rapidjson::Document d;
			d.Parse((char*)buf.data(), buf.size());

			if(!d.HasParseError()) {
				SPDLOG_DEBUG("Parsed");
				// TODO: Better validation

				// Reset stake data
				stakes.clear();
				cumulative_weights.clear();

				// Iterate through clusters
				auto& clusters = d["data"]["clusters"];
				for(rapidjson::SizeType i = 0; i < clusters.Size(); i++) {
					SPDLOG_DEBUG("Cluster {}", i);
					auto& cluster = clusters[i];
					uint64_t total_amount = 0;
					bool mpondThreshold = false;
					// Iterate through tokens
					auto& tokens = cluster["totalDelegations"];
					for(rapidjson::SizeType j = 0; j < tokens.Size(); j++) {
						SPDLOG_DEBUG("Delegation {}", j);
						auto& delegation = tokens[j];
						std::string amount = delegation["amount"].GetString();
						std::string token_id = delegation["token"]["tokenId"].GetString();
						if(token_id == "0x1635815984abab0dbb9afd77984dad69c24bf3d711bc0ddb1e2d53ef2d523e5e") {
							if(amount.size() > 12) {
								auto am = std::stoi(amount.substr(0, amount.size() - 12));
								total_amount += am;
								if(am >= 500000) {
									mpondThreshold = true;
								}
							}
						} else if(token_id == "0x5802add45f8ec0a524470683e7295faacc853f97cf4a8d3ffbaaf25ce0fd87c4") {
							if(amount.size() > 18) {
								auto am = std::stoi(amount.substr(0, amount.size() - 18));
								total_amount += am;
							}
						}
					}

					if(mpondThreshold) {
						std::string client_key_str = cluster["id"].GetString();
						std::array<uint8_t, 20> addr;
						for(uint i = 0; i < 20; i++) {
							uint8_t b = 0;
							auto byte = client_key_str[2*i+2];
							if(byte >= '0' && byte <= '9') b += byte - '0';
							else if (byte >= 'a' && byte <='f') b += byte - 'a' + 10;
							else if (byte >= 'A' && byte <='F') b += byte - 'A' + 10;
							b <<= 4;

							byte = client_key_str[2*i+3];
							if(byte >= '0' && byte <= '9') b += byte - '0';
							else if (byte >= 'a' && byte <='f') b += byte - 'a' + 10;
							else if (byte >= 'A' && byte <='F') b += byte - 'A' + 10;

							addr.data()[i] = b;
						}

						SPDLOG_DEBUG("Client key: 0x{:spn}, Stake: {}", spdlog::to_hex(addr.data(), addr.data()+addr.size()), total_amount);
						stakes[addr] = total_amount;
					}
				}

				uint64_t total = 0;
				cumulative_weights.reserve(stakes.size());
				for(auto iter = stakes.begin(); iter != stakes.end(); iter++) {
					total += (uint64_t)std::sqrt(iter->second);
					cumulative_weights.push_back(std::make_pair(total, iter->first));
					SPDLOG_DEBUG("Cumulative: {}", total);
				}
			}
			}
		}

		return 0;
	}

	template<typename FiberType>
	int did_dial(FiberType& fiber, core::SocketAddress addr [[maybe_unused]]) {
		using namespace std::string_literals;
		SPDLOG_DEBUG("StakeRequester: Did dial: {}", addr.to_string());
		fiber.template inner_call<"reset"_tag>(*this, 1000);
		auto query_str = "POST "s;
		query_str += staking_url;
		query_str += " HTTP/1.0\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: 180\r\n"
			"\r\n"
			"{\"query\": \"query { clusters(first: 1000, where: {networkId: \\\"";
		query_str += network_id;
		query_str += "\\\"}){id, totalDelegations{token{tokenId} amount}}}\"}"s;
		auto query = core::Buffer(query_str.size()).write_unsafe(
			0,
			(uint8_t*)query_str.c_str(),
			query_str.size()
		);
		fiber.template inner_call<"send"_tag>(*this, std::move(query));
		return 0;
	}

	template<typename FiberType>
	int did_send(FiberType&, core::Buffer&& buf [[maybe_unused]]) {
		SPDLOG_DEBUG("StakeRequester: Did send: {} bytes", buf.size());
		return 0;
	}

	int did_close(auto& fiber) {
		SPDLOG_DEBUG("Did close");
		delete &fiber;
		return 0;
	}

public:
	StakeRequester(std::tuple<std::string, std::string> args) : StakeRequester(std::get<0>(args), std::get<1>(args)) {}

	StakeRequester(std::string staking_url, std::string network_id) : staking_url(staking_url), network_id(network_id), t(this), dns_timer(this) {
		t.template start<StakeRequester, &StakeRequester::query_cb>(2000, 60000);
		dns_timer.template start<StakeRequester, &StakeRequester::dns_cb>(0, 60000);
	}

	std::unordered_map<std::array<uint8_t, 20>, uint64_t> stakes;
	std::vector<std::pair<uint64_t, std::array<uint8_t, 20>>> cumulative_weights;

	uint64_t request(std::array<uint8_t, 20> client_key) {
		auto iter = stakes.find(client_key);
		if(iter == stakes.end()) {
			return 0;
		}
		return iter->second;
	}

	std::vector<std::array<uint8_t, 20>> sample(uint64_t n = 1) {
		std::vector<std::array<uint8_t, 20>> samples;
		samples.reserve(n);

		if(cumulative_weights.size() <= n) {
			for(auto iter = cumulative_weights.begin(); iter != cumulative_weights.end(); iter++) {
				samples.push_back(iter->second);
			}
			return samples;
		}

		std::random_device rd;
		std::uniform_int_distribution<uint64_t> dist(0, cumulative_weights.back().first);

		while(samples.size() != 5) {
			auto rnd = dist(rd);
			auto iter = std::lower_bound(
				cumulative_weights.begin(),
				cumulative_weights.end(),
				rnd,
				[](std::pair<uint64_t, std::array<uint8_t, 20>> p, uint64_t v) { return p.first < v; }
			);

			if(std::find(samples.begin(), samples.end(), iter->second) != samples.end()) {
				continue;
			} else {
				samples.push_back(iter->second);
			}
		}

		return samples;
	}

};

struct MessageHeader {
	uint8_t const* attestation_data = nullptr;
	uint64_t attestation_size = 0;
	uint8_t const* witness_data = nullptr;
	uint64_t witness_size = 0;
};

//! Class containing the Pub-Sub functionality
/*!
	Uses the custom marlin-StreamTransport for message delivery

	Important functions:
	\li subscribe(publisher_address)
	\li unsubsribe(publisher_address)
	\li send_message_on_channel(channel, message)
*/
template<
	typename PubSubDelegate,
	bool enable_cut_through = false,
	bool accept_unsol_conn = false,
	bool enable_relay = false,
	typename AttesterType = EmptyAttester,
	typename WitnesserType = EmptyWitnesser,
	template <typename, typename...> class AbciTemplate = DefaultAbci
>
class PubSubNode {
private:
	size_t max_sol_conns;
	size_t max_unsol_conns;
	static constexpr uint64_t DefaultMsgIDTimerInterval = 10000;
	static constexpr uint64_t DefaultPeerSelectTimerInterval = 60000;
	static constexpr uint64_t DefaultBlacklistTimerInterval = 600000;
//---------------- Transport types ----------------//
public:
	using ClientKey = std::array<uint8_t, 20>;
	using Self = PubSubNode<
		PubSubDelegate,
		enable_cut_through,
		accept_unsol_conn,
		enable_relay,
		AttesterType,
		WitnesserType,
		AbciTemplate
	>;

	using MessageHeaderType = MessageHeader;

	template<typename ListenDelegate, typename TransportDelegate>
	using BaseStreamTransportFactory = stream::StreamTransportFactory<
		ListenDelegate,
		TransportDelegate,
		asyncio::UdpTransportFactory,
		asyncio::UdpTransport
	>;
	template<typename Delegate>
	using BaseStreamTransport = stream::StreamTransport<
		Delegate,
		asyncio::UdpTransport
	>;

	using BaseTransportFactory = lpf::LpfTransportFactory<
		Self,
		Self,
		BaseStreamTransportFactory,
		BaseStreamTransport,
		std::bool_constant<enable_cut_through>
	>;
	using BaseTransport = lpf::LpfTransport<
		Self,
		BaseStreamTransport,
		std::bool_constant<enable_cut_through>
	>;
	using AbciType = AbciTemplate<
		Self,
		uint64_t,
		uint16_t,
		MessageHeaderType,
		BaseTransport*
	>;
private:
	StakeRequester streq;
	AttesterType attester;
	WitnesserType witnesser;
	bool is_abci_active = false;
	AbciType abci;
// ---------------- Subscription management ----------------//
public:
	typedef PubSubTransportSet<BaseTransport> TransportSet;
	typedef std::unordered_map<uint16_t, TransportSet> TransportSetMap;

	// TransportSetMap channel_subscriptions;
	// TransportSetMap potential_channel_subscriptions;

	struct Connections {
		TransportSet sol_conns;
		TransportSet sol_standby_conns;
	};
	std::unordered_map<ClientKey, Connections> conn_map;
	TransportSet unsol_conns;
	std::unordered_map<core::SocketAddress, ClientKey> beacon_map;

	std::unordered_set<core::SocketAddress> blacklist_addr;
	// TransportSet unsol_standby_conns;

	void send_SUBSCRIBE(BaseTransport &transport, uint16_t channel);
	void send_UNSUBSCRIBE(BaseTransport &transport, uint16_t channel);

	bool add_sol_conn(ClientKey client_key, BaseTransport &transport);
	bool add_sol_standby_conn(ClientKey client_key, BaseTransport &transport);
	bool add_unsol_conn(BaseTransport &transport);
	// bool add_unsol_standby_conn(BaseTransport &transport); TODO: to be introduced later

	bool remove_conn(TransportSet &t_set, BaseTransport &Transport);

	// int get_num_active_subscribers(uint16_t channel);
	// void add_subscriber_to_channel(uint16_t channel, BaseTransport &transport);
	// void add_subscriber_to_potential_channel(uint16_t channel, BaseTransport &transport);
	// void remove_subscriber_from_channel(uint16_t channel, BaseTransport &transport);
	// void remove_subscriber_from_potential_channel(uint16_t channel, BaseTransport &transport);
private:
	asyncio::Timer peer_selection_timer;

	void peer_selection_timer_cb() {
		for(auto& [client_key, conns] : conn_map) {
			delegate->manage_subscriptions(client_key, max_sol_conns, conns.sol_conns, conns.sol_standby_conns);
		}

		for(auto& conn : unsol_conns) {
			[[maybe_unused]] auto* static_pk = conn->get_static_pk();
			[[maybe_unused]] auto* remote_static_pk = conn->get_remote_static_pk();
			SPDLOG_DEBUG(
				"Node {:spn}: Unsol conn: {:spn}: rtt: {}",
				spdlog::to_hex(static_pk, static_pk + crypto_box_PUBLICKEYBYTES),
				spdlog::to_hex(remote_static_pk, remote_static_pk + crypto_box_PUBLICKEYBYTES),
				conn->get_rtt()
			);
		}

		// std::for_each(
		//	this->delegate->channels.begin(),
		//	this->delegate->channels.end(),
		//	[&] (uint16_t channel) {
		//		this->delegate->manage_subscribers(channel, this->channel_subscriptions[channel], this->potential_channel_subscriptions[channel]);
		//	}
		// );
	}

	asyncio::Timer blacklist_timer;

	void blacklist_timer_cb() {
		this->blacklist_addr.clear();
	}

//---------------- Pubsub protocol ----------------//
private:
	BaseTransportFactory f;

	core::Buffer create_MESSAGE(
		uint16_t channel,
		uint64_t message_id,
		const uint8_t *data,
		uint64_t size,
		MessageHeaderType prev_header
	);

	int did_recv_SUBSCRIBE(BaseTransport &transport, core::Buffer &&message);
	void did_recv_UNSUBSCRIBE(BaseTransport &transport, core::Buffer &&message);
	void did_recv_RESPONSE(BaseTransport &transport, core::Buffer &&message);
	void send_RESPONSE(
		BaseTransport &transport,
		bool success,
		std::string msg_string
	);

	int did_recv_MESSAGE(BaseTransport &transport, core::Buffer &&message);
	void send_MESSAGE(
		BaseTransport &transport,
		uint16_t channel,
		uint64_t message_id,
		const uint8_t *data,
		uint64_t size,
		MessageHeaderType prev_header
	);

	void did_recv_HEARTBEAT(BaseTransport &transport, core::Buffer &&message);
	void send_HEARTBEAT(BaseTransport &transport);

//---------------- Base layer ----------------//
public:
	// Listen delegate
	bool should_accept(core::SocketAddress const &addr);
	void did_create_transport(BaseTransport &transport);

	// Abci delegate
	void did_connect(AbciType &);
	void did_disconnect(AbciType &);
	void did_close(AbciType &);
	int did_analyze_block(
		AbciType &,
		core::Buffer&&,
		std::string hash,
		std::string coinbase,
		core::WeakBuffer header,
		uint64_t message_id,
		uint16_t channel,
		MessageHeaderType message_header,
		BaseTransport *transport
	);

	// Transport delegate
	void did_dial(BaseTransport &transport);
	int did_recv(BaseTransport &transport, core::Buffer &&message);
	void did_send(BaseTransport &transport, core::Buffer &&message);
	void did_close(BaseTransport &transport, uint16_t reason);

	int dial(core::SocketAddress const &addr, uint8_t const *remote_static_pk);

//---------------- Public Interface ----------------//
public:
	template<
		typename ...AttesterArgs,
		typename ...WitnesserArgs,
		typename ...AbciArgs
	>
	PubSubNode(
		const core::SocketAddress &_addr,
		size_t max_sol,
		size_t max_unsol,
		uint8_t const *keys,
		std::tuple<std::string, std::string> req,
		std::tuple<AttesterArgs...> attester_args = {},
		std::tuple<WitnesserArgs...> witnesser_args = {},
		std::tuple<AbciArgs...> abci_args = {}
	);
	PubSubDelegate *delegate;

	uint64_t send_message_on_channel(
		uint16_t channel,
		const uint8_t *data,
		uint64_t size,
		core::SocketAddress const *excluded = nullptr
	);
	void send_message_on_channel(
		uint16_t channel,
		uint64_t message_id,
		const uint8_t *data,
		uint64_t size,
		core::SocketAddress const *excluded = nullptr,
		MessageHeaderType prev_header = {}
	);
	void send_message_on_channel_impl(
		uint16_t channel,
		uint64_t message_id,
		const uint8_t *data,
		uint64_t size,
		core::SocketAddress const *excluded = nullptr,
		MessageHeaderType prev_header = {}
	);
	void send_message_with_cut_through_check(
		BaseTransport *transport,
		uint16_t channel,
		uint64_t message_id,
		const uint8_t *data,
		uint64_t size,
		MessageHeaderType prev_header = {}
	);

	void subscribe(ClientKey client_key, core::SocketAddress const &addr, uint8_t const *remote_static_pk);
	void subscribe(core::SocketAddress const &addr, uint8_t const *remote_static_pk);
	void unsubscribe(core::SocketAddress const &addr);
private:
	template<
		typename ...AttesterArgs,
		typename ...WitnesserArgs,
		typename ...AbciArgs,
		size_t ...AI,
		size_t ...WI,
		size_t ...ABI
	>
	PubSubNode(
		const core::SocketAddress &_addr,
		size_t max_sol,
		size_t max_unsol,
		uint8_t const *keys,
		std::tuple<std::string, std::string> req,
		std::tuple<AttesterArgs...> attester_args,
		std::tuple<WitnesserArgs...> witnesser_args,
		std::tuple<AbciArgs...> abci_args,
		// Need the below args for tuple destructuring
		std::index_sequence<AI...>,
		std::index_sequence<WI...>,
		std::index_sequence<ABI...>
	);

//---------------- Message deduplication ----------------//
public:
private:
	std::uniform_int_distribution<uint64_t> message_id_dist;
	std::mt19937_64 message_id_gen;

	// Message id history for deduplication
	std::vector<std::vector<uint64_t>> message_id_events;
	uint8_t message_id_idx = 0;
	std::unordered_set<uint64_t> message_id_set;

	asyncio::Timer message_id_timer;

	void message_id_timer_cb() {
		// Overflow behaviour desirable
		this->message_id_idx++;

		for (
			auto iter = this->message_id_events[this->message_id_idx].begin();
			iter != this->message_id_events[this->message_id_idx].end();
			iter = this->message_id_events[this->message_id_idx].erase(iter)
		) {
			this->message_id_set.erase(*iter);
		}

		for(auto& [_, conns] : conn_map) {
			(void)_;
			for (auto* transport : conns.sol_conns) {
				this->send_HEARTBEAT(*transport);
			}

			for (auto* transport : conns.sol_standby_conns) {
				this->send_HEARTBEAT(*transport);
			}
		}
		// std::for_each(
		//	this->delegate->channels.begin(),
		//	this->delegate->channels.end(),
		//	[&] (uint16_t channel) {
		//		for (auto* transport : this->channel_subscriptions[channel]) {
		//			this->send_HEARTBEAT(*transport);
		//		}
		//		for (auto* pot_transport : this->potential_channel_subscriptions[channel]) {
		//			this->send_HEARTBEAT(*pot_transport);
		//		}
		//	}
		// );
	}

//---------------- Cut through ----------------//
public:
	void cut_through_recv_start(BaseTransport &transport, uint16_t id, uint64_t length);
	int cut_through_recv_bytes(BaseTransport &transport, uint16_t id, core::Buffer &&bytes);
	void cut_through_recv_end(BaseTransport &transport, uint16_t id);
	void cut_through_recv_flush(BaseTransport &transport, uint16_t id);
	void cut_through_recv_skip(BaseTransport &transport, uint16_t id);
private:
	struct pairhash {
		template <typename T, typename U>
		std::size_t operator()(const std::pair<T, U> &p) const
		{
			return std::hash<T>()(p.first) ^ std::hash<U>()(p.second);
		}
	};
	std::unordered_map<
		std::pair<BaseTransport *, uint16_t>,
		std::list<std::pair<BaseTransport *, uint16_t>>,
		pairhash
	> cut_through_map;
	std::unordered_map<
		std::pair<BaseTransport *, uint16_t>,
		uint64_t,
		pairhash
	> cut_through_length;
	std::unordered_map<
		std::pair<BaseTransport *, uint16_t>,
		bool,
		pairhash
	> cut_through_header_recv;

	uint8_t const* keys = nullptr;
};


// Impl

//---------------- Helper macros begin ----------------//

#define PUBSUBNODE_TEMPLATE typename PubSubDelegate, \
	bool enable_cut_through, \
	bool accept_unsol_conn, \
	bool enable_relay, \
	typename AttesterType, \
	typename WitnesserType, \
	template <typename, typename...> class Abci

#define PUBSUBNODETYPE PubSubNode< \
	PubSubDelegate, \
	enable_cut_through, \
	accept_unsol_conn, \
	enable_relay, \
	AttesterType, \
	WitnesserType, \
	Abci \
>

//---------------- Helper macros end ----------------//


//---------------- PubSub functions begin ----------------//

//! Callback on receipt of subscribe request
/*!
	\param transport StreamTransport instance to be added to subsriber list
	\param bytes Buffer containing the channel name to add the subscriber to
*/
template<PUBSUBNODE_TEMPLATE>
int PUBSUBNODETYPE::did_recv_SUBSCRIBE(
	BaseTransport &transport,
	core::Buffer &&bytes
) {
	// Bounds check
	if(bytes.size() < 2) {
		transport.close();
		return -1;
	}

	uint16_t channel [[maybe_unused]] = bytes.read_uint16_be_unsafe(0);

	SPDLOG_DEBUG(
		"Received subscribe on channel {} from {}",
		channel,
		transport.dst_addr.to_string()
	);

	// add_subscriber_to_channel(channel, transport);
	if (accept_unsol_conn) {

		if (blacklist_addr.find(transport.dst_addr) != blacklist_addr.end()) {
			blacklist_addr.erase(transport.dst_addr);
			add_sol_conn({}, transport);
			return 0;
		}

		add_unsol_conn(transport);
		if (!unsol_conns.check_tranport_in_set(transport)) {
			transport.close(1); // Add reason to indicate blacklist
			SPDLOG_DEBUG("CLOSING TRANSPORT, RETURNING -1");
			return -1;
		}
	}

	return 0;
}


/*!
	\verbatim

	SUBSCRIBE (0x00)
	Channel as payload.

	Message format:

	 0               1               2               3
	 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
	+++++++++++++++++++++++++++++++++
	|      0x00     |      0x00     |
	-----------------------------------------------------------------
	|                         Channel Name                        ...
	+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	\endverbatim
*/
template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::send_SUBSCRIBE(
	BaseTransport &transport,
	uint16_t channel
) {
	core::Buffer bytes({0}, 3);
	bytes.write_uint16_be_unsafe(1, channel);

	SPDLOG_DEBUG(
		"Sending subscribe on channel {} to {}",
		channel,
		transport.dst_addr.to_string()
	);

	transport.send(std::move(bytes));
}

//! Callback on receipt of unsubscribe request
/*!
	\param transport StreamTransport instance to be removed from subsriber list
	\param bytes Buffer containing the channel name to remove the subscriber from
*/
template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::did_recv_UNSUBSCRIBE(
	BaseTransport &transport,
	core::Buffer &&bytes
) {
	// Bounds check
	if(bytes.size() < 2) {
		transport.close();
		return;
	}

	uint16_t channel [[maybe_unused]] = bytes.read_uint16_be_unsafe(0);

	SPDLOG_DEBUG(
		"Received unsubscribe on channel {} from {}",
		channel,
		transport.dst_addr.to_string()
	);

	// TODO
	remove_conn(unsol_conns, transport);
}

/*!
	\verbatim

	UNSUBSCRIBE (0x01)
	Channel as payload.

	Message format:

	 0               1               2               3
	 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
	+++++++++++++++++++++++++++++++++
	|      0x00     |      0x01     |
	-----------------------------------------------------------------
	|                         Channel Name                        ...
	+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	\endverbatim
*/
template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::send_UNSUBSCRIBE(
	BaseTransport &transport,
	uint16_t channel
) {
	core::Buffer bytes({1}, 3);
	bytes.write_uint16_be_unsafe(1, channel);

	SPDLOG_DEBUG("Sending unsubscribe on channel {} to {}", channel, transport.dst_addr.to_string());

	transport.send(std::move(bytes));
}

/*!
    Callback on receipt of response
*/
template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::did_recv_RESPONSE(
	BaseTransport &transport,
	core::Buffer &&bytes
) {
	bool success [[maybe_unused]] = bytes.data()[0];

	// Bounds check on header
	if(bytes.size() < 1) {
		transport.close();
		return;
	}

	// Hide success
	bytes.cover_unsafe(1);

	// Process rest of the message
	std::string message(bytes.data(), bytes.data()+bytes.size());

	// Check subscribe/unsubscribe response
	if(message.rfind("UNSUBSCRIBED", 0) == 0) {
		delegate->did_unsubscribe(*this, delegate->channels[0]);
	} else if(message.rfind("SUBSCRIBED", 0) == 0) {
		delegate->did_subscribe(*this, delegate->channels[0]);
	}

	SPDLOG_DEBUG(
		"Received {} response: {}",
		success == 0 ? "ERROR" : "OK",
		spdlog::to_hex(message.data(), message.data() + message.size())
	);
}


/*!
	\verbatim

	RESPONSE (0x02)

	Payload contains a byte representing the reponse type(currently a OK/ERROR flag) followed by an arbitrary response message.

	Format:

	 0               1               2               3
	 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
	+++++++++++++++++++++++++++++++++++++++++++++++++
	|      0x00     |      0x02     |      Type     |
	-----------------------------------------------------------------
	|                            Message                          ...
	+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	\endverbatim
*/
template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::send_RESPONSE(
	BaseTransport &transport,
	bool success,
	std::string msg_string
) {
	// 0 for ERROR
	// 1 for OK
	core::Buffer m({2, static_cast<uint8_t>(success ? 1 : 0)}, msg_string.size()+2);
	m.write_unsafe(2, (uint8_t*)msg_string.data(), msg_string.size());

	SPDLOG_DEBUG(
		"Sending {} response: {}",
		success == 0 ? "ERROR" : "OK",
		spdlog::to_hex(m.data(), m.data() + msg_string.size()+2)
	);
	transport.send(std::move(m));
}

//! Callback on receipt of message data
/*!
	\li reassembles the fragmented packets received by the streamTransport back into meaninfull data component
	\li relay/forwards the message to other subscriptions on the channel if the enable_relay flag is true
	\li performs message deduplication i.e doesnt relay the message if it has already been relayed recently
*/
template<PUBSUBNODE_TEMPLATE>
int PUBSUBNODETYPE::did_recv_MESSAGE(
	BaseTransport &transport,
	core::Buffer &&bytes
) {
	// Bounds check on header
	if(bytes.size() < 10) {
		transport.close();
		return -1;
	}

	auto message_id = bytes.read_uint64_be_unsafe(0);
	auto channel = bytes.read_uint16_be_unsafe(8);

	SPDLOG_DEBUG("PUBSUBNODE did_recv_MESSAGE ### message id: {}, channel: {}", message_id, channel);

	bytes.cover_unsafe(10);
	MessageHeaderType header = {};

	auto att_opt = attester.parse_size(bytes, 0);
	if(!att_opt.has_value()) {
		SPDLOG_ERROR("Attestation size parse failure");
		transport.close();
		return -1;
	}

	header.attestation_data = bytes.data();
	header.attestation_size = att_opt.value();
	auto res = bytes.cover(header.attestation_size);

	if(!res) {
		SPDLOG_ERROR("Attestation too long: {}", header.attestation_size);
		transport.close();
		return -1;
	}

	auto wit_opt = witnesser.parse_size(bytes, 0);
	if(!wit_opt.has_value()) {
		SPDLOG_ERROR("Witness size parse failure");
		transport.close();
		return -1;
	}

	header.witness_data = bytes.data();
	header.witness_size = wit_opt.value();
	res = bytes.cover(header.witness_size);

	if(!res) {
		SPDLOG_ERROR("Witness too long: {}", header.witness_size);
		transport.close();
		return -1;
	}

	bool verify;
	std::array<uint8_t, 20> address;
	std::tie(verify, address) = attester.verify(message_id, channel, bytes.data(), bytes.size(), header);

	if(!verify) {
		SPDLOG_ERROR("Attestation verification failed");
		transport.close();
		return -1;
	}

	constexpr bool has_msg_log = requires(
		PubSubDelegate& d
	) {
		d.msg_log(core::SocketAddress(), std::array<uint8_t, 20>(), std::array<uint8_t, 20>(), message_id, bytes);
	};
	if constexpr(has_msg_log) {
		delegate->msg_log(transport.dst_addr, beacon_map[transport.dst_addr], address, message_id, bytes);
	}

	// Send it onward
	if(message_id_set.find(message_id) == message_id_set.end()) { // Deduplicate message
		message_id_set.insert(message_id);
		message_id_events[message_id_idx].push_back(message_id);

		if constexpr (enable_relay) {
			if(!transport.is_internal()) {
				if(is_abci_active) {
					abci.analyze_block(std::move(bytes), message_id, channel, header, &transport);
				} else {
					SPDLOG_ERROR("Abci not active, dropping block");
				}
			} else {
				send_message_on_channel_impl(
					channel,
					message_id,
					bytes.data(),
					bytes.size(),
					&transport.dst_addr,
					header
				);

				delegate->did_recv(
					*this,
					std::move(bytes),
					header,
					channel,
					message_id
				);
			}
		} else {
			delegate->did_recv(
				*this,
				std::move(bytes),
				header,
				channel,
				message_id
			);
		}
	}

	return 0;
}

template<PUBSUBNODE_TEMPLATE>
int PUBSUBNODETYPE::did_analyze_block(
	AbciType &,
	core::Buffer &&bytes,
	std::string,
	std::string,
	core::WeakBuffer,
	uint64_t message_id,
	uint16_t channel,
	MessageHeaderType message_header,
	BaseTransport *transport
) {
	// Relay to other peers.
	bool found = false;
	if(unsol_conns.check_tranport_in_set(*transport)) {
		found = true;
	} else {
		for(auto& [_, conns] : conn_map) {
			(void)_;
			if(
				conns.sol_conns.check_tranport_in_set(*transport) ||
				conns.sol_standby_conns.check_tranport_in_set(*transport)
			) {
				found = true;
				break;
			}
		}
	}
	if(!found) {
		return -1;
	}

	// Relay
	send_message_on_channel_impl(
		channel,
		message_id,
		bytes.data(),
		bytes.size(),
		&transport->dst_addr,
		message_header
	);

	// Call delegate.
	delegate->did_recv(
		*this,
		std::move(bytes),
		message_header,
		channel,
		message_id
	);

	return 0;
}


/*!
	\verbatim

	MESSAGE (0x03)

	Payload contains a 8 byte message length, a 8 byte message id, channel details and the message data.

	FORMAT:

	 0               1               2               3
	 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
	+++++++++++++++++++++++++++++++++
	|      0x00     |      0x03     |
	-----------------------------------------------------------------
	|                                                               |
	----                      Message Length                     ----
	|                                                               |
	-----------------------------------------------------------------
	|                                                               |
	----                        Message ID                       ----
	|                                                               |
	-----------------------------------------------------------------
	|      Channel name length      |
	-----------------------------------------------------------------
	|                         Channel Name                        ...
	-----------------------------------------------------------------
	|                         Message Data                        ...
	+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	\endverbatim
*/
template<PUBSUBNODE_TEMPLATE>
core::Buffer PUBSUBNODETYPE::create_MESSAGE(
	uint16_t channel,
	uint64_t message_id,
	const uint8_t *data,
	uint64_t size,
	MessageHeaderType prev_header
) {
	uint64_t buf_size = 11 + size;
	buf_size += attester.attestation_size(message_id, channel, data, size, prev_header);
	buf_size += witnesser.witness_size(prev_header);
	core::Buffer m({3}, buf_size);
	m.write_uint64_be_unsafe(1, message_id);
	m.write_uint16_be_unsafe(9, channel);

	uint64_t offset = 11;
	auto res = attester.attest(message_id, channel, data, size, prev_header, m, offset);
	if(res < 0) {
		SPDLOG_ERROR("Attestation failed: {}", res);
	}
	offset += attester.attestation_size(message_id, channel, data, size, prev_header);
	witnesser.witness(prev_header, m, offset);
	offset += witnesser.witness_size(prev_header);
	m.write_unsafe(offset, data, size);

	return m;
}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::send_MESSAGE(
	BaseTransport &transport,
	uint16_t channel,
	uint64_t message_id,
	const uint8_t *data,
	uint64_t size,
	MessageHeaderType prev_header
) {
	auto m = create_MESSAGE(
		channel,
		message_id,
		data,
		size,
		prev_header
	);
	transport.send(std::move(m));
}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::send_HEARTBEAT(
	BaseTransport &transport
) {
	core::Buffer m({4}, 1);

	transport.send(std::move(m));
}

//---------------- PubSub functions end ----------------//


//---------------- Listen delegate functions begin ----------------//

template<PUBSUBNODE_TEMPLATE>
bool PUBSUBNODETYPE::should_accept(core::SocketAddress const &) {
	return accept_unsol_conn;
}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::did_create_transport(
	BaseTransport &transport
) {
	SPDLOG_DEBUG(
		"DID CREATE TRANSPORT: {}",
		transport.dst_addr.to_string()
	);

	transport.setup(this, keys);
}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::did_connect(AbciType &) {
	SPDLOG_INFO("Did connect");
	is_abci_active = true;
}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::did_disconnect(AbciType &) {
	is_abci_active = false;
}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::did_close(AbciType &) {
	// Should never happen
	throw;
}


//---------------- Listen delegate functions end ----------------//


//---------------- Transport delegate functions begin ----------------//

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::did_dial(BaseTransport &transport) {

	SPDLOG_DEBUG(
		"DID DIAL: {}",
		transport.dst_addr.to_string()
	);

	add_sol_conn(beacon_map[transport.dst_addr], transport);
}

//! Receives the bytes/packet fragments from StreamTransport and processes them
/*!
	Determines the type of packet by reading the first byte and redirects the packet to appropriate function for further processing

	\verbatim

	first-byte	:	type
	0			:	subscribe
	1			:	unsubscribe
	2			:	response
	3			:	message

	\endverbatim
*/
template<PUBSUBNODE_TEMPLATE>
int PUBSUBNODETYPE::did_recv(
	BaseTransport &transport,
	core::Buffer &&bytes
) {
	// Abort on empty message
	if(bytes.size() == 0)
		return 0;

	// Bounds check on header
	if(bytes.size() < 1) {
		transport.close();
		return -1;
	}

	uint8_t message_type = bytes.data()[0];

	// Hide message type
	bytes.cover_unsafe(1);

	switch(message_type) {
		// SUBSCRIBE
		case 0: return this->did_recv_SUBSCRIBE(transport, std::move(bytes));
		break;
		// UNSUBSCRIBE
		case 1: this->did_recv_UNSUBSCRIBE(transport, std::move(bytes));
		break;
		// RESPONSE
		case 2: this->did_recv_RESPONSE(transport, std::move(bytes));
		break;
		// MESSAGE
		case 3: return this->did_recv_MESSAGE(transport, std::move(bytes));
		break;
		// HEARTBEAT, ignore
		case 4:
		break;
	}

	return 0;
}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::did_send(
	BaseTransport &,
	core::Buffer &&
) {}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::did_close(BaseTransport &transport, uint16_t reason) {
	// Remove from subscribers
	// std::for_each(
	//	delegate->channels.begin(),
	//	delegate->channels.end(),
	//	[&] (uint16_t channel) {
	//		channel_subscriptions[channel].erase(&transport);
	//		potential_channel_subscriptions[channel].erase(&transport);
	//	}
	// );

	beacon_map.erase(transport.dst_addr);
	for(auto& [client_key, conns] : conn_map) {
		bool is_sol = remove_conn(conns.sol_conns, transport) || remove_conn(conns.sol_standby_conns, transport);
		if (is_sol && reason == 1) {
			// add to blacklist
			blacklist_addr.insert(transport.dst_addr);
		}

		remove_conn(unsol_conns, transport);

		// Flush subscribers
		for(auto id : transport.cut_through_used_ids) {
			for(auto& [subscriber, subscriber_id] : cut_through_map[std::make_pair(&transport, id)]) {
				subscriber->cut_through_send_flush(subscriber_id);
			}

			cut_through_map.erase(std::make_pair(&transport, id));
		}

		// Call Manage_subscribers to rebalance lists
		if(is_sol)
			delegate->manage_subscriptions(client_key, max_sol_conns, conns.sol_conns, conns.sol_standby_conns);
	}

	// Remove subscriptions
	for(auto& [_, subscribers] : cut_through_map) {
		(void)_;
		for (auto iter = subscribers.begin(); iter != subscribers.end();) {
			if(iter->first == &transport) {
				iter = subscribers.erase(iter);
				break;
			} else {
				iter++;
			}
		}
	}
}

//---------------- Transport delegate functions end ----------------//

template<PUBSUBNODE_TEMPLATE>
template<
	typename ...AttesterArgs,
	typename ...WitnesserArgs,
	typename ...AbciArgs
>
PUBSUBNODETYPE::PubSubNode(
	const core::SocketAddress &addr,
	size_t max_sol,
	size_t max_unsol,
	uint8_t const* keys,
	std::tuple<std::string, std::string> req,
	std::tuple<AttesterArgs...> attester_args,
	std::tuple<WitnesserArgs...> witnesser_args,
	std::tuple<AbciArgs...> abci_args
) : PubSubNode(
	addr,
	max_sol,
	max_unsol,
	keys,
	std::move(req),
	std::move(attester_args),
	std::move(witnesser_args),
	std::move(abci_args),
	std::index_sequence_for<AttesterArgs...>{},
	std::index_sequence_for<WitnesserArgs...>{},
	std::index_sequence_for<AbciArgs...>{}
) {}

template<PUBSUBNODE_TEMPLATE>
template<
	typename ...AttesterArgs,
	typename ...WitnesserArgs,
	typename ...AbciArgs,
	size_t ...AI,
	size_t ...WI,
	size_t ...ABI
>
PUBSUBNODETYPE::PubSubNode(
	const core::SocketAddress &addr,
	size_t max_sol,
	size_t max_unsol,
	uint8_t const* keys,
	std::tuple<std::string, std::string> req,
	std::tuple<AttesterArgs...> attester_args [[maybe_unused]],
	std::tuple<WitnesserArgs...> witnesser_args [[maybe_unused]],
	std::tuple<AbciArgs...> abci_args [[maybe_unused]],
	std::index_sequence<AI...>,
	std::index_sequence<WI...>,
	std::index_sequence<ABI...>
) : max_sol_conns(max_sol),
	max_unsol_conns(max_unsol),
	streq(std::move(req)),
	attester(std::get<AI>(attester_args)...),
	witnesser(std::get<WI>(witnesser_args)...),
	abci(this, std::get<ABI>(abci_args)...),
	peer_selection_timer(this),
	blacklist_timer(this),
	message_id_gen(std::random_device()()),
	message_id_events(256),
	message_id_timer(this),
	keys(keys)
{
	f.bind(addr);
	f.listen(*this);
	message_id_timer.template start<Self, &Self::message_id_timer_cb>(DefaultMsgIDTimerInterval, DefaultMsgIDTimerInterval);
	peer_selection_timer.template start<Self, &Self::peer_selection_timer_cb>(DefaultPeerSelectTimerInterval, DefaultPeerSelectTimerInterval);
	blacklist_timer.template start<Self, &Self::blacklist_timer_cb>(DefaultBlacklistTimerInterval, DefaultBlacklistTimerInterval);

	streq.request({});
}

template<PUBSUBNODE_TEMPLATE>
int PUBSUBNODETYPE::dial(core::SocketAddress const &addr, uint8_t const *remote_static_pk) {
	SPDLOG_DEBUG(
		"SENDING DIAL TO: {}",
		addr.to_string()
	);

	return f.dial(addr, *this, remote_static_pk);
}


//! sends messages over a given channel
/*!
	\param channel name of channel to send message on
	\param data uint8_t* byte sequence of message to send
	\param size of the message to send
	\param excluded avoid this address while sending the message
*/
template<PUBSUBNODE_TEMPLATE>
uint64_t PUBSUBNODETYPE::send_message_on_channel(
	uint16_t channel,
	const uint8_t *data,
	uint64_t size,
	core::SocketAddress const *excluded
) {
	uint64_t message_id = this->message_id_dist(this->message_id_gen);
	send_message_on_channel_impl(channel, message_id, data, size, excluded);

	return message_id;
}

//! sends messages over a given channel
/*!
	\param channel name of channel to send message on
	\param message_id msg id
	\param data uint8_t* byte sequence of message to send
	\param size of the message to send
	\param excluded avoid this address while sending the message
*/
template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::send_message_on_channel(
	uint16_t channel,
	uint64_t message_id,
	const uint8_t *data,
	uint64_t size,
	core::SocketAddress const *excluded,
	MessageHeaderType prev_header
) {
	if(message_id_set.find(message_id) != message_id_set.end()) { // Deduplicate message
		return;
	} else {
		message_id_set.insert(message_id);
		message_id_events[message_id_idx].push_back(message_id);
	}

	send_message_on_channel_impl(channel, message_id, data, size, excluded, prev_header);
}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::send_message_on_channel_impl(
	uint16_t channel,
	uint64_t message_id,
	const uint8_t *data,
	uint64_t size,
	core::SocketAddress const *excluded,
	MessageHeaderType prev_header
) {
	if(conn_map.size() <= 5) {
		for(auto& [client_key, conns] : conn_map) {
			SPDLOG_DEBUG("Sending message {} to 0x{:spn}", message_id, spdlog::to_hex(client_key.data(), client_key.data()+client_key.size()));

			for (
				auto it = conns.sol_conns.begin();
				it != conns.sol_conns.end();
				it++
			) {
				// Exclude given address, usually sender tp prevent loops
				if(excluded != nullptr && (*it)->dst_addr == *excluded)
					continue;
				send_message_with_cut_through_check(*it, channel, message_id, data, size, prev_header);
			}
		}
	} else {
		auto cks = streq.sample(5);
		for(auto& client_key : cks) {
			auto& conns = conn_map[client_key];
			SPDLOG_INFO("Sending message {} to 0x{:spn}", message_id, spdlog::to_hex(client_key.data(), client_key.data()+client_key.size()));

			for (
				auto it = conns.sol_conns.begin();
				it != conns.sol_conns.end();
				it++
			) {
				// Exclude given address, usually sender tp prevent loops
				if(excluded != nullptr && (*it)->dst_addr == *excluded)
					continue;
				send_message_with_cut_through_check(*it, channel, message_id, data, size, prev_header);
			}
		}
	}

	for (
		auto it = unsol_conns.begin();
		it != unsol_conns.end();
		it++
	) {
		// Exclude given address, usually sender tp prevent loops
		if(excluded != nullptr && (*it)->dst_addr == *excluded)
			continue;
		send_message_with_cut_through_check(*it, channel, message_id, data, size, prev_header);
	}
}


template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::send_message_with_cut_through_check(
	BaseTransport *transport,
	uint16_t channel,
	uint64_t message_id,
	const uint8_t *data,
	uint64_t size,
	MessageHeaderType prev_header
) {
	SPDLOG_DEBUG(
		"Sending message {} on channel {} to {}",
		message_id,
		channel,
		transport->dst_addr.to_string()
	);

	if(size > 50000) {
		auto m = create_MESSAGE(
			channel,
			message_id,
			data,
			size,
			prev_header
		);

		auto res = transport->cut_through_send(std::move(m));

		// TODO: Handle better
		if(res < 0) {
			SPDLOG_ERROR("Cut through send failed");
			transport->close();
		}
	} else {
		send_MESSAGE(*transport, channel, message_id, data, size, prev_header);
	}
}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::subscribe(
	ClientKey client_key,
	core::SocketAddress const &addr,
	uint8_t const *remote_static_pk
) {
	// TODO: written so that relays with full unsol list dont occupy sol/standby lists in clients, and similarly masters with full unsol list dont occupy sol/standby lists in relays
	if (blacklist_addr.find(addr) != blacklist_addr.end())
		return;

	auto *transport = f.get_transport(addr);

	if(transport == nullptr) {
		beacon_map[addr] = client_key;
		dial(addr, remote_static_pk);
		return;
	} else if(!transport->is_active()) {
		return;
	}

	add_sol_conn(client_key, *transport);
}

//! subscribes to given publisher
/*!
	\param addr publisher address to subscribe to
*/
template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::subscribe(core::SocketAddress const &addr, uint8_t const *remote_static_pk) {
	subscribe({}, addr, remote_static_pk);
}

//! unsubscribes from given publisher
/*!
	\param addr publisher address to unsubscribe from
*/
template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::unsubscribe(core::SocketAddress const &addr) {
	auto *transport = f.get_transport(addr);

	if(transport == nullptr) {
		return;
	}

	std::for_each(
		delegate->channels.begin(),
		delegate->channels.end(),
		[&] (uint16_t channel) {
			send_UNSUBSCRIBE(*transport, channel);
		}
	);
}

template<PUBSUBNODE_TEMPLATE>
bool PUBSUBNODETYPE::add_sol_conn(ClientKey client_key, BaseTransport &transport) {
	SPDLOG_DEBUG("add sol: {}, {}", spdlog::to_hex(client_key.data(), client_key.data()+client_key.size()), transport.dst_addr.to_string());
	auto& conns = conn_map[client_key];

	//TODO: size check.
	if (conns.sol_conns.size() >= max_sol_conns) {
		add_sol_standby_conn(client_key, transport);
		return false;
	}

	remove_conn(conns.sol_standby_conns, transport);
	remove_conn(unsol_conns, transport);

	if (!conns.sol_conns.check_tranport_in_set(transport)) {
		std::for_each(
			delegate->channels.begin(),
			delegate->channels.end(),
			[&] (uint16_t channel) {
				send_SUBSCRIBE(transport, channel);
			}
		);

		SPDLOG_DEBUG("Adding address: {} to sol conn list",
			transport.dst_addr.to_string()
		);

		conns.sol_conns.insert(&transport);
		//TODO: send response
		send_RESPONSE(transport, true, "SUBSCRIBED");

		return true;
	}

	return false;
}

template<PUBSUBNODE_TEMPLATE>
bool PUBSUBNODETYPE::add_sol_standby_conn(ClientKey client_key, BaseTransport &transport) {
	auto& conns = conn_map[client_key];

	remove_conn(unsol_conns, transport);

	if(!conns.sol_conns.check_tranport_in_set(transport) &&
		!conns.sol_standby_conns.check_tranport_in_set(transport)) {

		SPDLOG_DEBUG("Adding address: {} to sol standby conn list",
			transport.dst_addr.to_string()
		);

		conns.sol_standby_conns.insert(&transport);
		return true;
	}

	return false;
}

template<PUBSUBNODE_TEMPLATE>
bool PUBSUBNODETYPE::add_unsol_conn(BaseTransport &transport) {
	if (unsol_conns.size() >= max_unsol_conns) {
		return false;
	}

	if(!unsol_conns.check_tranport_in_set(transport)) {

		SPDLOG_DEBUG("Adding address: {} to unsol conn list",
			transport.dst_addr.to_string()
		);

		unsol_conns.insert(&transport);

		send_RESPONSE(transport, true, "SUBSCRIBED");

		return true;
	}

	return false;
}

template<PUBSUBNODE_TEMPLATE>
bool PUBSUBNODETYPE::remove_conn(TransportSet &t_set, BaseTransport &transport) {

	if (t_set.check_tranport_in_set(transport)) {
		SPDLOG_DEBUG("Removing address: {} from list",
			transport.dst_addr.to_string()
		);

		t_set.erase(&transport);

		return true;
	}

	return false;
}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::cut_through_recv_start(
	BaseTransport &transport,
	uint16_t id,
	uint64_t length
) {
	cut_through_map[std::make_pair(&transport, id)] = {};
	cut_through_header_recv[std::make_pair(&transport, id)] = false;
	cut_through_length[std::make_pair(&transport, id)] = length;

	SPDLOG_DEBUG(
		"Pubsub {} <<<< {}: CTR start: {}",
		transport.src_addr.to_string(),
		transport.dst_addr.to_string(),
		id
	);
}

template<PUBSUBNODE_TEMPLATE>
int PUBSUBNODETYPE::cut_through_recv_bytes(
	BaseTransport &transport,
	uint16_t id,
	core::Buffer &&bytes
) {
	SPDLOG_DEBUG(
		"Pubsub {} <<<< {}: CTR recv: {}, {}",
		transport.src_addr.to_string(),
		transport.dst_addr.to_string(),
		id,
		bytes.size()
	);
	if(!cut_through_header_recv[std::make_pair(&transport, id)]) {
		// Bounds check on header
		if(bytes.size() < 11) {
			SPDLOG_ERROR("Not enough header: {}, {}", bytes.size(), 11);
			transport.close();
			return -1;
		}

		auto message_id = bytes.read_uint64_be_unsafe(1);
		auto channel = bytes.read_uint16_be_unsafe(9);

		cut_through_header_recv[std::make_pair(&transport, id)] = true;

		if(message_id_set.find(message_id) == message_id_set.end()) { // Deduplicate message
			message_id_set.insert(message_id);
			message_id_events[message_id_idx].push_back(message_id);
		} else {
			// transport.cut_through_send_skip(id);
			return 0;
		}

		SPDLOG_INFO(
			"Pubsub {} <<<< {}: CTR message id: {}",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			message_id
		);

		size_t offset = 11;
		MessageHeaderType header = {};

		auto att_opt = attester.parse_size(bytes, offset);
		if(!att_opt.has_value()) {
			SPDLOG_ERROR("Attestation size parse failure");
			transport.close();
			return -1;
		}

		header.attestation_data = bytes.data() + offset;
		header.attestation_size = att_opt.value();
		offset += header.attestation_size;

		if(bytes.size() < offset) {
			SPDLOG_ERROR("Attestation too long: {}", header.attestation_size);
			transport.close();
			return -1;
		}

		auto wit_opt = witnesser.parse_size(bytes, offset);
		if(!wit_opt.has_value()) {
			SPDLOG_ERROR("Witness size parse failure");
			transport.close();
			return -1;
		}

		header.witness_data = bytes.data() + offset;
		header.witness_size = wit_opt.value();
		offset += header.witness_size;

		if(bytes.size() < offset) {
			SPDLOG_ERROR("Witness too long: {}", header.witness_size);
			transport.close();
			return -1;
		}

		SPDLOG_DEBUG(
			"Pubsub {} <<<< {}: CTR witness: {}",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			spdlog::to_hex(header.witness_data, header.witness_data + header.witness_size)
		);

		bool verify;
		std::array<uint8_t, 20> address;
		std::tie(verify, address) = attester.verify(message_id, channel, bytes.data() + offset, bytes.size() - offset, header);

		if(!verify) {
			SPDLOG_ERROR("Attestation verification failed");
			transport.close();
			return -1;
		}

		for(auto& [_, conns] : conn_map) {
			(void)_;
			for(auto *subscriber : conns.sol_conns) {
				if(&transport == subscriber) continue;
				bool found = witnesser.contains(header, subscriber->get_remote_static_pk());
				if (found) continue;

				auto sub_id = subscriber->cut_through_send_start(
					cut_through_length[std::make_pair(&transport, id)]
				);
				if(sub_id == 0) {
					SPDLOG_ERROR("Cannot send to subscriber");
					continue;
				}

				cut_through_map[std::make_pair(&transport, id)].push_back(
					std::make_pair(subscriber, sub_id)
				);
			}
		}

		for(auto *subscriber : unsol_conns) {
			if(&transport == subscriber) continue;
			bool found = witnesser.contains(header, subscriber->get_remote_static_pk());
			if (found) continue;

			auto sub_id = subscriber->cut_through_send_start(
				cut_through_length[std::make_pair(&transport, id)]
			);
			if(sub_id == 0) {
				SPDLOG_ERROR("Cannot send to subscriber");
				continue;
			}

			cut_through_map[std::make_pair(&transport, id)].push_back(
				std::make_pair(subscriber, sub_id)
			);
		}

		witnesser.witness(header, bytes, 11 + header.attestation_size);

		return cut_through_recv_bytes(transport, id, std::move(bytes));
	} else {
		for(auto [subscriber, sub_id] : cut_through_map[std::make_pair(&transport, id)]) {
			auto sub_bytes = core::Buffer(bytes.size());
			sub_bytes.write_unsafe(0, bytes.data(), bytes.size());

			auto res = subscriber->cut_through_send_bytes(sub_id, std::move(sub_bytes));

			// TODO: Handle better
			if(res < 0) {
				SPDLOG_ERROR("Cut through send failed");
				subscriber->close();
			}
		}
	}

	return 0;
}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::cut_through_recv_end(
	BaseTransport &transport,
	uint16_t id
) {
	for(auto [subscriber, sub_id] : cut_through_map[std::make_pair(&transport, id)]) {
		subscriber->cut_through_send_end(sub_id);
	}
	SPDLOG_DEBUG(
		"Pubsub {} <<<< {}: CTR end: {}",
		transport.src_addr.to_string(),
		transport.dst_addr.to_string(),
		id
	);
}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::cut_through_recv_flush(
	BaseTransport &transport,
	uint16_t id
) {
	for(auto [subscriber, sub_id] : cut_through_map[std::make_pair(&transport, id)]) {
		subscriber->cut_through_send_flush(sub_id);
	}
	SPDLOG_DEBUG(
		"Pubsub {} <<<< {}: CTR flush: {}",
		transport.src_addr.to_string(),
		transport.dst_addr.to_string(),
		id
	);
}

template<PUBSUBNODE_TEMPLATE>
void PUBSUBNODETYPE::cut_through_recv_skip(
	BaseTransport &transport,
	uint16_t id
) {
	// Remove subscriptions
	for(auto& [_, subscribers] : cut_through_map) {
		(void)_;
		for (auto iter = subscribers.begin(); iter != subscribers.end();) {
			if(iter->first == &transport && iter->second == id) {
				iter = subscribers.erase(iter);
				break;
			} else {
				iter++;
			}
		}
	}
	SPDLOG_DEBUG(
		"Pubsub {} <<<< {}: CTR skip: {}",
		transport.src_addr.to_string(),
		transport.dst_addr.to_string(),
		id
	);
}

//---------------- Helper macros undef begin ----------------//

#undef PUBSUBNODE_TEMPLATE
#undef PUBSUBNODETYPE

//---------------- Helper macros undef end ----------------//

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_PUBSUBNODE_HPP
