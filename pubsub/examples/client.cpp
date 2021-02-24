#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <cstring>
#include <algorithm>
#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/pubsub/witness/BloomWitnesser.hpp>
#include <marlin/pubsub/attestation/EmptyAttester.hpp>
#include <sodium.h>

using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::stream;
using namespace marlin::pubsub;
using namespace std;

class PubSubNodeDelegate {
private:
	using PubSubNodeType = PubSubNode<PubSubNodeDelegate, true, true, true, EmptyAttester, BloomWitnesser>;

public:
	std::vector<uint16_t> channels = {100, 101};

	void did_unsubscribe(PubSubNodeType &, uint16_t channel) {
		SPDLOG_INFO("Did unsubscribe: {}", channel);
	}

	void did_subscribe(PubSubNodeType &ps, uint16_t channel) {
		uint8_t message[100000];
		ps.send_message_on_channel(channel, message, 100000);
		SPDLOG_INFO("Did subscribe: {}", channel);
	}

	void did_recv(
		PubSubNodeType &,
		Buffer &&,
		typename PubSubNodeType::MessageHeaderType header,
		uint16_t channel,
		uint64_t message_id
	) {
		SPDLOG_INFO(
			"Received message {:spn} on channel {} with witness {}",
			spdlog::to_hex((uint8_t*)&message_id, ((uint8_t*)&message_id) + 8),
			channel,
			spdlog::to_hex(header.witness_data, header.witness_data+header.witness_size)
		);
	}

	void manage_subscriptions(
		std::array<uint8_t, 20>,
		size_t,
		typename PubSubNodeType::TransportSet&,
		typename PubSubNodeType::TransportSet&
	) {

	}
};

int main() {
	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];

	crypto_box_keypair(static_pk, static_sk);

	PubSubNodeDelegate b_del;

	size_t max_sol_conn = 10;
	size_t max_unsol_conn = 1;

	auto addr = SocketAddress::from_string("127.0.0.1:8000");

	using PBNT = PubSubNode<PubSubNodeDelegate, true, true, true, EmptyAttester, BloomWitnesser>;

	auto b = new PBNT(addr, max_sol_conn, max_unsol_conn, static_sk, std::forward_as_tuple("", ""), {}, std::tie(static_pk));
	b->delegate = &b_del;

	auto addr2 = SocketAddress::from_string("127.0.0.1:8001");
	auto b2 = new PBNT(addr2, max_sol_conn, max_unsol_conn, static_sk, std::forward_as_tuple("", ""), {}, std::tie(static_pk));
	b2->delegate = &b_del;

	SPDLOG_INFO("Start");

	b->dial(addr2, static_pk);

	return EventLoop::run();
}
