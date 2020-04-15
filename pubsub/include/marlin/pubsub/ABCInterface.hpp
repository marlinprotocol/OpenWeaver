#ifndef MARLIN_PUBSUB_ABCINTERFACE_HPP
#define MARLIN_PUBSUB_ABCINTERFACE_HPP

#include <fstream>
#include <experimental/filesystem>
#include <sodium.h>
#include <unistd.h>

#include <marlin/asyncio/tcp/TcpTransportFactory.hpp>
#include <marlin/core/BN.hpp>
#include <marlin/stream/StreamTransportFactory.hpp>
#include <marlin/lpf/LpfTransportFactory.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>

namespace marlin {
namespace pubsub {

const uint8_t addressLength = 20;
const uint8_t balanceLength = 32;
const uint8_t privateKeyLength = 32;
const uint8_t messageTypeSize = 1;
const uint8_t AckTypeSize = 1;
const uint8_t blockNumberSize = 8;

const uint8_t  messageIDSize = 8;
const uint8_t channelSize = 2;
const uint8_t timestampSize = 8 ;
const uint8_t stakeOffsetSize = 8;
const uint8_t messageSize = 8;
const uint8_t hashSize = 32;
const uint8_t signatureSize = 65;

const uint64_t staleThreshold = 30000;

class ABCInterface;

const bool enable_cut_through = false;
using LpfTcpTransportFactory = lpf::LpfTransportFactory<
	ABCInterface,
	ABCInterface,
	asyncio::TcpTransportFactory,
	asyncio::TcpTransport,
	enable_cut_through
>;
using LpfTcpTransport = lpf::LpfTransport<
	ABCInterface,
	asyncio::TcpTransport,
	enable_cut_through
>;

class ABCInterface {
	uint8_t private_key[32] = {};
	bool have_key = false;

	std::map<std::string, uint64_t> stakeAddressMap;
	uint64_t lastUpdateTime;
	uint64_t latestBlockReceived;

public:
	LpfTcpTransportFactory f;
	LpfTcpTransport* contractInterface = nullptr;
	// timestamp
	// db

	ABCInterface() {
		f.bind(core::SocketAddress());
		f.dial(core::SocketAddress::from_string("127.0.0.1:7000"), *this);
		lastUpdateTime = 0;
		latestBlockReceived = 0;
	}

	//-----------------------delegates for Lpf-Tcp-Transport-----------------------------------

	// Listen delegate: Not required
	bool should_accept(core::SocketAddress const &) {
		return true;
	}

	void did_create_transport(LpfTcpTransport &transport) {
		SPDLOG_DEBUG(
			"DID CREATE LPF TRANSPORT: {}",
			transport.dst_addr.to_string()
		);

		transport.setup(this);

		contractInterface = &transport;
	}

	// Transport delegate

	void did_dial(LpfTcpTransport &transport [[maybe_unused]]) {
		SPDLOG_DEBUG(
			"DID DIAL: {}",
			transport.dst_addr.to_string()
		);
	}

	enum MessageType {
		StateUpdateMessage = 1,
		AckMessage = 2,
		PrivateKeyMessage = 3,
		DuplicateStakeMessage = 4
	};

	// TODO Size checks and null checks while reading messages from buffer
	int did_recv_message(
		LpfTcpTransport &transport,
		core::Buffer &&message
	) {
		SPDLOG_INFO(
			"Did recv from blockchain client, message with length {}",
			message.size()
		);

		// Bounds check on header
		if(message.size() < 1) {
			return -1;
		}

		auto messageType = message.read_uint8(0);
		message.cover_unsafe(1);

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
		LpfTcpTransport &transport,
		core::Buffer &message
	) {
		// Bounds check
		if(message.size() < 12) {
			return;
		}

		auto blockNumber = message.read_uint64_be(0);
		message.cover_unsafe(blockNumberSize);

		if (blockNumber > latestBlockReceived || latestBlockReceived == 0) {
			auto numMapEntries = message.read_uint32_be(0);
			message.cover_unsafe(4);

			SPDLOG_INFO(
				"BlockNumber {}",
				blockNumber
			);

			SPDLOG_INFO(
				"NumEntries {}",
				numMapEntries
			);

			for (uint i=0; i<numMapEntries; i++) {
				// Bounds check
				if(message.size() < 28) {
					return;
				}

				std::string addressString((char*)message.data(), addressLength);
				message.cover_unsafe(addressLength);

				SPDLOG_INFO(
					"address {}",
					addressString
				);

				// uint64_t hi = message.read_uint64_be(0);
				// message.cover(8);
				// uint64_t hilo = message.read_uint64_be(0);
				// message.cover(8);
				// uint64_t lohi = message.read_uint64_be(0);
				// message.cover(8);
				// uint64_t lo = message.read_uint64_be(0);
				// message.cover(8);

				// auto balance = core::uint256_t(lo, lohi, hilo, hi);

				uint64_t balance = message.read_uint64_be(0);
				message.cover_unsafe(8);

				SPDLOG_INFO(
					"balance {} ",
					balance
				);

				stakeAddressMap[addressString] = balance;

			}

			latestBlockReceived = blockNumber;
			lastUpdateTime = asyncio::EventLoop::now();
		}

		send_ack_state_update(transport, latestBlockReceived);
	}

	void did_recv_private_key(
		LpfTcpTransport &,
		core::Buffer &message
	) {
		// Bounds check
		if(message.size() < privateKeyLength) {
			return;
		}

		message.read_unsafe(0, private_key, privateKeyLength);
		have_key = true;
	}

	void send_ack_state_update(
		LpfTcpTransport &transport,
		uint64_t blockNumber
	) {

		auto messageType = MessageType::AckMessage;
		uint8_t ackType = MessageType::StateUpdateMessage;

		uint32_t totalSize = messageTypeSize + AckTypeSize + blockNumberSize;
		auto dataBuffer = new core::Buffer(totalSize);
		uint32_t offset = 0;

		dataBuffer->write_uint8(offset, messageType);
		offset += messageTypeSize;
		dataBuffer->write_uint8(offset, ackType);
		offset += AckTypeSize;
		dataBuffer->write_uint64_be(offset, blockNumber);
		offset += blockNumberSize;

		transport.send(std::move(*dataBuffer));
	}


	// |  Message Id |  Channel Id |  Timestamp  | StakeOffset | MessageSize |  Chunk Hash  | Attestation Signature |

	void send_duplicate_stake_msg(
		uint64_t messageId1,
		uint16_t chId1,
		uint64_t timestamp1,
		uint64_t stakeOffset1,
		uint64_t messageSize1,
	 	uint8_t* messageHash1,
		uint8_t* signature1,
		uint64_t messageId2,
		uint16_t chId2,
		uint64_t timestamp2,
		uint64_t stakeOffset2,
		uint64_t messageSize2,
		uint8_t* messageHash2,
		uint8_t* signature2
	) {
		if (contractInterface == nullptr)
			return;

		auto messageType = MessageType::DuplicateStakeMessage;

		uint32_t mSize = messageIDSize + channelSize + timestampSize + stakeOffsetSize + messageSize + hashSize + signatureSize;

		uint32_t totalSize = messageTypeSize + 2 * (mSize);
		auto dataBuffer = new core::Buffer(totalSize);
		uint32_t offset = 0;

		dataBuffer->write_uint8(offset, messageType);
		offset += messageTypeSize;

		dataBuffer->write_uint64_be(offset, messageId1);
		offset += messageIDSize;

		dataBuffer->write_uint16_be(offset, chId1);
		offset += channelSize;

		dataBuffer->write_uint64_be(offset, timestamp1);
		offset += timestampSize;

		dataBuffer->write_uint64_be(offset, stakeOffset1);
		offset += stakeOffsetSize;

		dataBuffer->write_uint64_be(offset, messageSize1);
		offset += messageSize;

		dataBuffer->write_unsafe(offset, messageHash1, hashSize);
		offset += hashSize;

		dataBuffer->write_unsafe(offset, signature1, signatureSize);
		offset += signatureSize;

		dataBuffer->write_uint64_be(offset, messageId2);
		offset += messageIDSize;

		dataBuffer->write_uint16_be(offset, chId2);
		offset += channelSize;

		dataBuffer->write_uint64_be(offset, timestamp2);
		offset += timestampSize;

		dataBuffer->write_uint64_be(offset, stakeOffset2);
		offset += stakeOffsetSize;

		dataBuffer->write_uint64_be(offset, messageSize2);
		offset += messageSize;

		dataBuffer->write_unsafe(offset, messageHash2, hashSize);
		offset += hashSize;

		dataBuffer->write_unsafe(offset, signature2, signatureSize);
		offset += signatureSize;

		contractInterface->send(std::move(*dataBuffer));
	}

	void did_send_message(
		LpfTcpTransport &,
		core::Buffer &&
	) {}

	void did_close(LpfTcpTransport &transport  [[maybe_unused]]) {
		SPDLOG_DEBUG(
			"Closed connection with client: {}",
			transport.dst_addr.to_string()
		);

		contractInterface = nullptr;
	}

	bool is_alive() {
 		return (asyncio::EventLoop::now() - lastUpdateTime) < staleThreshold;
	}

	uint64_t get_stake(std::string address) {
		if (! is_alive() || (stakeAddressMap.find(address) == stakeAddressMap.end()))
			return 0;
		return stakeAddressMap[address];
	}

	uint8_t* get_key() {
		if (have_key)
			return private_key;
		return nullptr;
	}

};

} // namespace pubsub
} // namespace marlin

#endif // MARLIN_PUBSUB_ABCINTERFACE_HPP
