#ifndef MARLIN_ONRAMP_ETH_ONRAMP_HPP
#define MARLIN_ONRAMP_ETH_ONRAMP_HPP

#include <marlin/pubsub/PubSubNode.hpp>
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
	marlin::beacon::DiscoveryClient<OnRamp> *b;
	marlin::pubsub::PubSubNode<OnRamp> *ps;
	marlin::rlpx::RlpxTransport<OnRamp> *rlpxt;

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
			ps->add_subscriber(addr);
		}
	}

	std::vector<std::string> channels = {"eth"};

	void did_unsubscribe(
		marlin::pubsub::PubSubNode<OnRamp> &,
		std::string channel
	) {
		SPDLOG_INFO("Did unsubscribe: {}", channel);
	}

	void did_subscribe(
		marlin::pubsub::PubSubNode<OnRamp> &,
		std::string channel
	) {
		SPDLOG_INFO("Did subscribe: {}", channel);
	}

	void did_recv_message(
		marlin::pubsub::PubSubNode<OnRamp> &,
		std::unique_ptr<char[]> &&message,
		uint64_t size,
		std::string &channel,
		uint64_t message_id
	) {
		SPDLOG_INFO(
			"Received message {} on channel {}",
			message_id,
			channel
		);

		if(rlpxt) {
			rlpxt->send(marlin::net::Buffer(message.release(), size));
		}
	}

	void did_recv_message(RlpxTransport<OnRamp> &transport, Buffer &&message) {
		CryptoPP::BLAKE2b blake2b((uint)8);
		blake2b.Update((uint8_t *)message.data(), message.size());
		uint64_t message_id;
		blake2b.TruncatedFinal((uint8_t *)&message_id, 8);
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did recv message {}: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			message_id,
			message.size()
		);

		if(message.data()[0] == 0x10) { // eth63 Status
			Buffer response(new char[72], 72);

			std::memcpy(response.data(), message.data(), 5);
			response.data()[2] = 0x45;
			response.data()[5] = 0x01;
			std::memcpy(response.data() + 6, message.data() + 42, 33);
			std::memcpy(response.data() + 39, message.data() + 42, 33);

			transport.send(std::move(response));
		} else if(message.data()[0] == 0x13) { // eth63 GetBlockHeaders
			transport.send(Buffer(new char[2]{0x14, (char)0xc0}, 2));
		} else if(message.data()[0] == 0x15) { // eth63 GetBlockBodies
			transport.send(Buffer(new char[2]{0x16, (char)0xc0}, 2));
		} else if(message.data()[0] == 0x1d) { // eth63 GetNodeData
			transport.send(Buffer(new char[2]{0x1e, (char)0xc0}, 2));
		} else if(message.data()[0] == 0x12) { // eth63 Transactions
			ps->send_message_on_channel(
				"eth",
				message_id,
				message.data(),
				message.size()
			);
		} else if(message.data()[0] == 0x17) { // eth63 NewBlock
			ps->send_message_on_channel(
				"eth",
				message_id,
				message.data(),
				message.size()
			);
		}
	}

	void did_send_message(RlpxTransport<OnRamp> &transport, Buffer &&message) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did send message: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			message.size()
		);
	}

	void did_dial(RlpxTransport<OnRamp> &transport) {
		(void)transport;
		// transport.send(Buffer(new char[10], 10));
	}

	bool should_accept(SocketAddress const &) {
		return true;
	}

	void did_create_transport(RlpxTransport<OnRamp> &transport) {
		rlpxt  = &transport;
		transport.setup(this);
	}
};

#endif // MARLIN_ONRAMP_ETH_ONRAMP_HPP
