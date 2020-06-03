#ifndef MARLIN_ONRAMP_ETH_ONRAMP_HPP
#define MARLIN_ONRAMP_ETH_ONRAMP_HPP

#include <marlin/net/udp/UdpTransport.hpp>
#include <marlin/stream/StreamTransportHelper.hpp>
#include <marlin/pubsub/PubSubClient.hpp>
#include <marlin/beacon/DiscoveryClient.hpp>
#include <marlin/rlpx/RlpxTransportFactory.hpp>
#include <cryptopp/blake2.h>

#define PUBSUB_PROTOCOL_NUMBER 0x10000000

using namespace marlin;
using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;
using namespace marlin::rlpx;

class OnRamp {
public:
	DiscoveryClient<OnRamp> *b;
	PubSubClient<OnRamp> *ps;
	RlpxTransport<OnRamp> *rlpxt = nullptr;

	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {};
	}

	void new_peer(
		net::SocketAddress const &addr,
		uint32_t protocol,
		uint16_t
	) {
		if(protocol == PUBSUB_PROTOCOL_NUMBER) {
			ps->subscribe(addr);

			// TODO: Better design
			// ps->add_subscriber(addr);
		}
	}

	std::vector<std::string> channels = {"eth"};

	void did_unsubscribe(
		PubSubClient<OnRamp> &,
		std::string channel __attribute__((unused))
	) {
		SPDLOG_DEBUG("Did unsubscribe: {}", channel);
	}

	void did_subscribe(
		PubSubClient<OnRamp> &,
		std::string channel __attribute__((unused))
	) {
		SPDLOG_DEBUG("Did subscribe: {}", channel);
	}

	void did_recv_message(
		PubSubClient<OnRamp> &,
		Buffer &&message,
		std::string &channel __attribute__((unused)),
		uint64_t message_id __attribute__((unused))
	) {
		if ((message_id & 0x0) == 0) {
			SPDLOG_INFO(
				"Received message {} on channel {}",
				message_id,
				channel
			);
		}

		if(rlpxt) {
			rlpxt->send(std::move(message));
		}
	}

	void did_recv_message(RlpxTransport<OnRamp> &transport, Buffer &&message) {
		CryptoPP::BLAKE2b blake2b((uint)8);
		blake2b.Update((uint8_t *)message.data(), message.size());
		uint64_t message_id;
		blake2b.TruncatedFinal((uint8_t *)&message_id, 8);
		SPDLOG_DEBUG(
			"Transport {{ Src: {}, Dst: {} }}: Did recv message {}: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			message_id,
			message.size()
		);

		if(message.data()[0] == 0x10) { // eth63 Status
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: Status message: {} bytes: {}",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				spdlog::to_hex(message.data(), message.data() + message.size())
			);

			Buffer response(new char[77], 77);

			std::memcpy(response.data(), message.data(), 7);
			response.data()[2] = 74;
			std::memcpy(response.data()+7, "\x83\x02\x00\x00", 4);
			std::memcpy(response.data() + 11, message.data() + message.size() - 33, 33);
			std::memcpy(response.data() + 44, message.data() + message.size() - 33, 33);

			transport.send(std::move(response));
		} else if(message.data()[0] == 0x13) { // eth63 GetBlockHeaders
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: GetBlockHeaders message: {} bytes",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				message.size()
			);
			transport.send(Buffer(new char[2]{0x14, (char)0xc0}, 2));
		} else if(message.data()[0] == 0x15) { // eth63 GetBlockBodies
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: GetBlockBodies message: {} bytes",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				message.size()
			);
			transport.send(Buffer(new char[2]{0x16, (char)0xc0}, 2));
		} else if(message.data()[0] == 0x1d) { // eth63 GetNodeData
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: GetNodeData message: {} bytes",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				message.size()
			);
			transport.send(Buffer(new char[2]{0x1e, (char)0xc0}, 2));
		} else if(message.data()[0] == 0x12) { // eth63 Transactions
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: Transactions message: {} bytes",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				message.size()
			);
			ps->send_message_on_channel(
				"eth",
				message_id,
				message.data(),
				message.size()
			);
		} else if(message.data()[0] == 0x17) { // eth63 NewBlock
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: NewBlock message: {} bytes",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				message.size()
			);
			ps->send_message_on_channel(
				"eth",
				message_id,
				message.data(),
				message.size()
			);
		} else {
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: Unknown message: {} bytes: {}",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				spdlog::to_hex(message.data(), message.data() + message.size())
			);
		}
	}

	void did_send_message(
		RlpxTransport<OnRamp> &transport __attribute__((unused)),
		Buffer &&message __attribute__((unused))
	) {
		SPDLOG_DEBUG(
			"Transport {{ Src: {}, Dst: {} }}: Did send message: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			message.size()
		);
	}

	void did_dial(RlpxTransport<OnRamp> &) {}

	bool should_accept(SocketAddress const &) {
		return true;
	}

	void did_create_transport(RlpxTransport<OnRamp> &transport) {
		rlpxt  = &transport;
		transport.setup(this);
	}

	void manage_subscribers(
		std::string channel,
		typename PubSubClient<OnRamp>::TransportSet& transport_set,
		typename PubSubClient<OnRamp>::TransportSet& potential_transport_set
	) {
		SPDLOG_INFO(
			"TS: {}, PTS: {}",
			transport_set.size(),
			potential_transport_set.size()
		);

		// move some of the subscribers to potential subscribers if oversubscribed
		if (transport_set.size() >= DefaultMaxSubscriptions) {
			// insert churn algorithm here. need to find a better algorithm to give old bad performers a chance gain. Pick randomly from potential peers?
			// send message to removed and added peers

			auto* toReplaceTransport = transport_set.find_max_rtt_transport();
			auto* toReplaceWithTransport = potential_transport_set.find_random_rtt_transport();

			if (toReplaceTransport != nullptr &&
				toReplaceWithTransport != nullptr) {

				SPDLOG_INFO(
					"Moving address: {} from subscribers to potential subscribers list on channel: {} ",
					toReplaceTransport->dst_addr.to_string(),
					channel
				);

				ps->remove_subscriber_from_channel(channel, *toReplaceTransport);
				ps->add_subscriber_to_potential_channel(channel, *toReplaceTransport);

				SPDLOG_INFO(
					"Moving address: {} from potential subscribers to subscribers list on channel: {} ",
					toReplaceWithTransport->dst_addr.to_string(),
					channel
				);

				ps->remove_subscriber_from_potential_channel(channel, *toReplaceWithTransport);
				ps->add_subscriber_to_channel(channel, *toReplaceWithTransport);
			}
		}

		for (auto* pot_transport __attribute__((unused)) : potential_transport_set) {
			SPDLOG_INFO("Potential Subscriber: {}  rtt: {} on channel {}", pot_transport->dst_addr.to_string(), pot_transport->get_rtt(), channel);
		}

		for (auto* transport __attribute__((unused)) : transport_set) {
			SPDLOG_INFO("Subscriber: {}  rtt: {} on channel {}", transport->dst_addr.to_string(),  transport->get_rtt(), channel);
		}
	}
};

#endif // MARLIN_ONRAMP_ETH_ONRAMP_HPP
