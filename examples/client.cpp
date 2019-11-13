#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <uv.h>
#include <cstring>
#include <algorithm>
#include <marlin/pubsub/PubSubServer.hpp>
#include <marlin/pubsub/PubSubClient.hpp>


using namespace marlin::net;
using namespace marlin::stream;
using namespace marlin::pubsub;
using namespace std;


class PubSubServerDelegate {
public:
	std::vector<std::string> channels = {"some_channel", "other_channel"};

	void did_unsubscribe(PubSubServer<PubSubServerDelegate> &, std::string channel) {
		SPDLOG_INFO("Did unsubscribe: {}", channel);
	}

	void did_subscribe(PubSubServer<PubSubServerDelegate> &, std::string channel) {
		SPDLOG_INFO("Did subscribe: {}", channel);
	}

	void did_recv_message(PubSubServer<PubSubServerDelegate> &, Buffer &&message, std::string &channel, uint64_t message_id) {
		SPDLOG_INFO("Received message {} on channel {}: {}", message_id, channel, spdlog::to_hex(message.data(), message.data() + message.size()));
	}

	void manage_subscribers(
		std::string,
		typename PubSubServer<PubSubServerDelegate>::TransportSet&,
		typename PubSubServer<PubSubServerDelegate>::TransportSet&
	) {

	}
};

class PubSubClientDelegate {
public:
	std::vector<std::string> channels = {"some_channel", "other_channel"};

	void did_unsubscribe(PubSubClient<PubSubClientDelegate> &, std::string channel) {
		SPDLOG_INFO("Did unsubscribe: {}", channel);
	}

	void did_subscribe(PubSubClient<PubSubClientDelegate> &, std::string channel) {
		SPDLOG_INFO("Did subscribe: {}", channel);
	}

	void did_recv_message(PubSubClient<PubSubClientDelegate> &, Buffer &&message, std::string &channel, uint64_t message_id) {
		SPDLOG_INFO("Received message {} on channel {}: {}", message_id, channel, spdlog::to_hex(message.data(), message.data() + message.size()));
	}

	void manage_subscribers(
		std::string,
		typename PubSubClient<PubSubClientDelegate>::TransportSet&,
		typename PubSubClient<PubSubClientDelegate>::TransportSet&
	) {

	}
};

int main() {
	PubSubClientDelegate b_del;
	PubSubServerDelegate b2_del;

	auto addr = SocketAddress::from_string("127.0.0.1:8000");
	auto b = new PubSubClient<PubSubClientDelegate>(addr);
	b->delegate = &b_del;

	auto addr2 = SocketAddress::from_string("127.0.0.1:8001");
	auto b2 = new PubSubServer<PubSubServerDelegate>(addr2);
	b2->delegate = &b2_del;

	SPDLOG_INFO("Start");

	b->dial(addr2);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
