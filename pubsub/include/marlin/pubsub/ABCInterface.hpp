/*
	1. loop to retry connection to contract interface on restart broken connection
	2. maps to store balance
	3. Null checks
	4. Acks
	5. Convert to server?
	6. int for balance
*/
#ifndef MARLIN_PUBSUB_ABCINTERFACE_HPP
#define MARLIN_PUBSUB_ABCINTERFACE_HPP

#include <fstream>
#include <experimental/filesystem>
#include <sodium.h>
#include <unistd.h>

#include <marlin/net/tcp/TcpTransportFactory.hpp>
#include <marlin/stream/StreamTransportFactory.hpp>
#include <marlin/lpf/LpfTransportFactory.hpp>
#include <marlin/net/core/EventLoop.hpp>

namespace marlin {
namespace pubsub {

std::string hexStr(char *data, int len)
{
	char* hexchar = new char[2*len + 1];
	int i;
	for (i=0; i<len; i++) {
		sprintf(hexchar+i*2, "%02hhx", data[i]);
	}
	hexchar[i*2] = 0;

	std::string hexstring = std::string(hexchar);
	delete[] hexchar;

	return hexstring;
}

const uint8_t addressLength = 20;
const uint8_t balanceLength = 32;
const uint8_t privateKeyLength = 32;
const uint8_t messageTypeSize = 1;
const uint8_t AckTypeSize = 1;
const uint8_t blockNumberSize = 8;
const uint64_t staleThreshold = 30000;

class ABCInterface;

const bool enable_cut_through = false;
using LpfTcpTransportFactory = lpf::LpfTransportFactory<
	ABCInterface,
	ABCInterface,
	net::TcpTransportFactory,
	net::TcpTransport,
	enable_cut_through
>;
using LpfTcpTransport = lpf::LpfTransport<
	ABCInterface,
	net::TcpTransport,
	enable_cut_through
>;

class ABCInterface {

std::string privateKey;
std::map<std::string, std::string> stakeAddressMap;
uint64_t lastUpdateTime;
uint64_t latestBlockReceived;

public:
	LpfTcpTransportFactory f;
	LpfTcpTransport* contractInterface = nullptr;
	// timestamp
	// db

	ABCInterface() {
		f.bind(net::SocketAddress());
		dial(net::SocketAddress::from_string("127.0.0.1:7000"), NULL);
		lastUpdateTime = 0;
		latestBlockReceived = 0;
	}

	//-----------------------delegates for Lpf-Tcp-Transport-----------------------------------

	// Listen delegate: Not required
	bool should_accept(net::SocketAddress const &addr [[maybe_unused]]) {
		return true;
	}

	void did_create_transport(LpfTcpTransport &transport) {
		SPDLOG_DEBUG(
			"DID CREATE LPF TRANSPORT: {}",
			transport.dst_addr.to_string()
		);

		transport.setup(this, NULL);

		contractInterface = &transport;
	}

	// Transport delegate

	// TODO?
	void did_dial(LpfTcpTransport &transport [[maybe_unused]]) {
		SPDLOG_DEBUG(
			"DID DIAL: {}",
			transport.dst_addr.to_string()
		);
	}

	enum MessageType {
		StateUpdateMessage = 1,
		AckMessage = 2,
		PrivateKeyMessage = 3
	};

	// TODO Size checks and null checks while reading messages from buffer
	int did_recv_message(
		LpfTcpTransport &transport [[maybe_unused]],
		net::Buffer &&message  [[maybe_unused]]
	) {
		SPDLOG_INFO(
			"Did recv from blockchain client, message with length {}",
			message.size()
		);

		auto messageType = message.read_uint8(0);
		message.cover(1);

		switch (messageType) {
			case MessageType::StateUpdateMessage:
				did_recv_state_update(transport, message);
				break;

			case MessageType::PrivateKeyMessage:
				did_recv_private_key(transport, message);
				break;
		}

		return 0;
	}


	void did_recv_state_update (
		LpfTcpTransport &transport [[maybe_unused]],
		net::Buffer &message  [[maybe_unused]]
	) {

		auto blockNumber = message.read_uint64_be(0);
		message.cover(blockNumberSize);

		if (blockNumber > latestBlockReceived || latestBlockReceived == 0) {
			auto numMapEntries = message.read_uint32_be(0);
			message.cover(4);

			SPDLOG_INFO(
				"BlockNumber {}",
				blockNumber
			);

			SPDLOG_INFO(
				"NumEntries {}",
				numMapEntries
			);

			for (uint i=0; i<numMapEntries; i++) {

				std::string addressString = hexStr(message.data(), addressLength);
				message.cover(addressLength);

				std::string balanceString = hexStr(message.data(), balanceLength);
				message.cover(balanceLength);

				stakeAddressMap[addressString] = balanceString;

				SPDLOG_INFO(
					"address {}",
					addressString
				);

				SPDLOG_INFO(
					"balance {}",
					balanceString
				);
			}

			latestBlockReceived = blockNumber;
			lastUpdateTime = net::EventLoop::now();
		}

		send_ack_state_update(transport, latestBlockReceived);
	}

	void did_recv_private_key(
		LpfTcpTransport &transport [[maybe_unused]],
		net::Buffer &message  [[maybe_unused]]
	) {
		privateKey = hexStr(message.data(), privateKeyLength);
		SPDLOG_INFO(
			"private key {}",
			privateKey
		);
	}

	void send_ack_state_update(
		LpfTcpTransport &transport [[maybe_unused]],
		uint64_t blockNumber) {

		auto messageType = MessageType::AckMessage;
		uint8_t ackType = MessageType::StateUpdateMessage;

		uint32_t totalSize = messageTypeSize + AckTypeSize + blockNumberSize;
		auto dataBuffer = new net::Buffer(new char[totalSize], totalSize);
		uint32_t offset = 0;

		dataBuffer->write_uint8(offset, messageType);
		offset += messageTypeSize;
		dataBuffer->write_uint8(offset, ackType);
		offset += AckTypeSize;
		dataBuffer->write_uint64_be(offset, blockNumber);
		offset += blockNumberSize;

		transport.send(std::move(*dataBuffer));
	}

	void did_send_message(
		LpfTcpTransport &transport [[maybe_unused]],
		net::Buffer &&message [[maybe_unused]]
	) {}

	// TODO:
	void did_close(LpfTcpTransport &transport  [[maybe_unused]]) {
		SPDLOG_DEBUG(
			"Closed connection with client: {}",
			transport.dst_addr.to_string()
		);

		contractInterface = nullptr;
	}

	int dial(net::SocketAddress const &addr, uint8_t const *remote_static_pk) {
		SPDLOG_INFO(
			"SENDING DIAL TO: {}",
			addr.to_string()
		);

		return f.dial(addr, *this, remote_static_pk);
	};


	void send_message(
		std::string channel [[maybe_unused]],
		uint64_t message_id [[maybe_unused]],
		const char *data [[maybe_unused]]
	) {}

	// Enquiry calls by
	//checkBalance(xyz)
	bool is_alive() {
 		// return (EventLoop::now() - lastUpdateTime) > staleThreshold;
		return true;
	}

	// TODO check if entry is new
	std::string getStake(std::string address) {
		if (! is_alive() || (stakeAddressMap.find(address) == stakeAddressMap.end()))
			return "";
		return stakeAddressMap[address];
	}
};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_ABCINTERFACE_HPP
