#ifndef MARLIN_ONRAMP_ETH_ONRAMP_HPP
#define MARLIN_ONRAMP_ETH_ONRAMP_HPP

#include <marlin/multicast/DefaultMulticastClient.hpp>
#include <marlin/rlpx/RlpxTransportFactory.hpp>
#include <cryptopp/blake2.h>

#define PUBSUB_PROTOCOL_NUMBER 0x10000000

using namespace marlin;
using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::beacon;
using namespace marlin::pubsub;
using namespace marlin::rlpx;
using namespace marlin::multicast;

class OnRamp {
public:
	DefaultMulticastClient<OnRamp> multicastClient;
	RlpxTransport<OnRamp> *rlpxt = nullptr;
	RlpxTransportFactory<OnRamp, OnRamp> f;

	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {};
	}

	OnRamp(DefaultMulticastClientOptions clop) : multicastClient(clop) {
		f.bind(SocketAddress::loopback_ipv4(12121));
		f.listen(*this);
	}

	template<typename T> // TODO: Code smell, remove later
	void did_recv_message(
		DefaultMulticastClient<OnRamp> &,
		Buffer &&message,
		T,
		uint16_t channel,
		uint64_t message_id
	) {
		SPDLOG_DEBUG(
			"Did recv from multicast, message-id: {}",
			message_id
		);

		if (channel == 0 && rlpxt != nullptr) {
			SPDLOG_DEBUG(
				"Sending to blockchain client"
			);
			rlpxt->send(std::move(message));
		}
	}

	void did_subscribe(
		DefaultMulticastClient<OnRamp> &,
		uint16_t
	) {}

	void did_unsubscribe(
		DefaultMulticastClient<OnRamp> &,
		uint16_t
	) {}

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

			Buffer response(77);

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
			transport.send(Buffer({0x14, 0xc0}, 2));
		} else if(message.data()[0] == 0x15) { // eth63 GetBlockBodies
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: GetBlockBodies message: {} bytes",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				message.size()
			);
			transport.send(Buffer({0x16, 0xc0}, 2));
		} else if(message.data()[0] == 0x1d) { // eth63 GetNodeData
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: GetNodeData message: {} bytes",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				message.size()
			);
			transport.send(Buffer({0x1e, 0xc0}, 2));
		} else if(message.data()[0] == 0x12) { // eth63 Transactions
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: Transactions message: {} bytes",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				message.size()
			);
			multicastClient.ps.send_message_on_channel(
				0,
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
			multicastClient.ps.send_message_on_channel(
				0,
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
};

#endif // MARLIN_ONRAMP_ETH_ONRAMP_HPP
