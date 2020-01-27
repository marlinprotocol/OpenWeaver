#include <marlin/multicast/DefaultMulticastClient.hpp>


using namespace marlin::multicast;
using namespace marlin::net;

class MulticastDelegate {
public:
	void did_recv_message(
		DefaultMulticastClient<MulticastDelegate> &client,
		Buffer &&message,
		Buffer &&witness,
		std::string &channel,
		uint64_t message_id
	) {
		SPDLOG_INFO(
			"Did recv message: {}",
			message_id
		);
	}

	void did_subscribe(
		DefaultMulticastClient<MulticastDelegate> &client,
		std::string &channel
	) {}

	void did_unsubscribe(
		DefaultMulticastClient<MulticastDelegate> &client,
		std::string &channel
	) {}
};

constexpr uint msg_rate = 1;
constexpr uint msg_size = 500;

void msggen_timer_cb(uv_timer_t *handle) {
	auto &client = *(DefaultMulticastClient<MulticastDelegate> *)handle->data;

	char msg[msg_size];

	for (uint i = 0; i < msg_rate; ++i) {
		client.ps.send_message_on_channel(
			"eth",
			msg,
			msg_size
		);
	}
}

int main(int , char **argv) {
	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);

	MulticastDelegate del;

	DefaultMulticastClientOptions clop {
		static_sk,
		{"eth"},
		std::string(argv[1]),
		"0.0.0.0:15002",
		"0.0.0.0:15000"
	};
	DefaultMulticastClient<MulticastDelegate> cl(clop);
	cl.delegate = &del;

	uv_timer_t msggen_timer;
	msggen_timer.data = &cl;
	uv_timer_init(uv_default_loop(), &msggen_timer);
	uv_timer_start(&msggen_timer, msggen_timer_cb, 100, 100);

	return DefaultMulticastClient<MulticastDelegate>::run_event_loop();
}
