#include <marlin/core/Keystore.hpp>
#include <marlin/multicast/DefaultMulticastClient.hpp>
#include <random>
#include <marlin/pubsub/attestation/SigAttester.hpp>

// #include <structopt/app.hpp>

using namespace marlin::multicast;
using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::pubsub;

class MulticastDelegate {
public:
	template<typename T> // TODO: Code smell, remove later
	void did_recv(
		DefaultMulticastClient<MulticastDelegate, SigAttester, LpfBloomWitnesser> &client,
		Buffer &&message,
		T header,
		uint16_t channel,
		uint64_t message_id
	) {
		if((message_id & 0x0) == 0) {
			SPDLOG_INFO(
				"Received message {} on channel {}",
				message_id,
				channel
			);
		}
	}

	void did_subscribe(
		DefaultMulticastClient<MulticastDelegate, SigAttester, LpfBloomWitnesser> &client,
		uint16_t channel
	) {}

	void did_unsubscribe(
		DefaultMulticastClient<MulticastDelegate, SigAttester, LpfBloomWitnesser> &client,
		uint16_t channel
	) {}
};

constexpr uint msg_rate = 1;
constexpr uint msg_size = 500000;
constexpr float randomness = 1;

std::mt19937 gen((std::random_device())());

uint8_t msg[msg_size];

void msggen_timer_cb(uv_timer_t *handle) {
	auto &client = *(DefaultMulticastClient<MulticastDelegate, SigAttester, LpfBloomWitnesser> *)handle->data;

	if(std::generate_canonical<float, 10>(gen) > randomness) {
		return;
	}

	for (uint i = 0; i < msg_rate; ++i) {
		auto message_id = client.ps.send_message_on_channel(
			0,
			msg,
			msg_size
		);
		if((message_id & 0x0) == 0) {
			SPDLOG_INFO(
				"Received message {} on channel {}",
				message_id,
				0
			);
		}
	}
}


int main(int , char **argv) {
	for (uint i = 0; i < msg_size; ++i)
	{
		msg[i] = (uint8_t)i;
	}

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);

	MulticastDelegate del;

	auto key = get_key("/home/roshan/Downloads/nitinkeystore", "/home/roshan/Downloads/nitinpass");
	DefaultMulticastClientOptions clop {
		static_sk,
		static_pk,
		{0},
		std::string(argv[1]),
		"0.0.0.0:15002",
		"0.0.0.0:15000"
	};
	DefaultMulticastClient<MulticastDelegate, SigAttester, LpfBloomWitnesser> cl(clop, (uint8_t*)key.data());
	cl.delegate = &del;

	uv_timer_t msggen_timer;
	msggen_timer.data = &cl;
	uv_timer_init(uv_default_loop(), &msggen_timer);
	uv_timer_start(&msggen_timer, msggen_timer_cb, 1000, 1000);

	return DefaultMulticastClient<MulticastDelegate, SigAttester, LpfBloomWitnesser>::run_event_loop();
}

