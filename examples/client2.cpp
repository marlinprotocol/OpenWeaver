#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <uv.h>
#include <cstring>
#include <algorithm>
#include <marlin/pubsub/PubSubNode.hpp>
#include <sodium.h>


using namespace marlin::net;
using namespace marlin::stream;
using namespace marlin::pubsub;
using namespace std;


class PubSubNodeDelegate {
private:
	using PubSubNodeType = PubSubNode<PubSubNodeDelegate, true, true, true>;

public:
	std::vector<std::string> channels = {"some_channel", "other_channel"};

	void did_unsubscribe(PubSubNodeType &, std::string channel) {
		SPDLOG_INFO("Did unsubscribe: {}", channel);
	}

	void did_subscribe(PubSubNodeType &, std::string channel) {
		SPDLOG_INFO("Did subscribe: {}", channel);
	}

	void did_recv_message(PubSubNodeType &, Buffer &&message, std::string &channel, uint64_t message_id) {
		SPDLOG_INFO("Received message {} on channel {}: {}", message_id, channel, spdlog::to_hex(message.data(), message.data() + message.size()));
	}

	void manage_subscriptions(
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

	auto addr = SocketAddress::from_string("127.0.0.1:8000");
	auto b = new PubSubNode<PubSubNodeDelegate, true, true, true>(addr, max_sol_conn, static_sk);
	b->delegate = &b_del;

	auto addr2 = SocketAddress::from_string("127.0.0.1:8001");
	auto b2 = new PubSubNode<PubSubNodeDelegate, true, true, true>(addr2, max_sol_conn, static_sk);
	b2->delegate = &b_del;

	SPDLOG_INFO("Start");

	b->dial(addr2, static_pk);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
