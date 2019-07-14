#ifndef MARLIN_ONRAMP_ETH_ONRAMP_HPP
#define MARLIN_ONRAMP_ETH_ONRAMP_HPP

#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/beacon/Beacon.hpp>
#include <marlin/rlpx/RlpxTransportFactory.hpp>

using namespace marlin;
using namespace marlin::net;
using namespace marlin::beacon;
using namespace marlin::pubsub;
using namespace marlin::rlpx;

class OnRamp {
public:
	marlin::beacon::Beacon<OnRamp> *b;
	marlin::pubsub::PubSubNode<OnRamp> *ps;
	marlin::rlpx::RlpxTransport<OnRamp> *rlpxt;

	void handle_new_peer(const net::SocketAddress &addr) {
		net::SocketAddress ps_addr(addr);

		// Set correct port.
		// TODO: Improve discovery to avoid hardcoding
		reinterpret_cast<sockaddr_in *>(&ps_addr)->sin_port = htons(8000);

		ps->subscribe(ps_addr);
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
			"Received message {} on channel {}: {}",
			message_id,
			channel,
			spdlog::to_hex(message.get(), message.get() + size)
		);

		if(rlpxt) {
			rlpxt->send(marlin::net::Buffer(message.release(), size));
		}
	}

	void did_recv_message(RlpxTransport<OnRamp> &transport, Buffer &&message) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did recv message: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			message.size()
		);

		if(message.data()[0] == 0x10) { // eth63 Status
			transport.send(std::move(message));
		} else if(message.data()[0] == 0x13) { // eth63 GetBlockHeaders
			transport.send(Buffer(new char[2]{0x14, (char)0xc0}, 2));
		} else if(message.data()[0] == 0x15) { // eth63 GetBlockBodies
			transport.send(Buffer(new char[2]{0x16, (char)0xc0}, 2));
		} else if(message.data()[0] == 0x12) { // eth63 Transactions
			ps->send_message_on_channel(
				"eth",
				message.data(),
				message.size()
			);
		} else if(message.data()[0] == 0x17) { // eth63 NewBlock
			ps->send_message_on_channel(
				"eth",
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
