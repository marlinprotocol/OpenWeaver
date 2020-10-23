#include <marlin/near/OnRampNear.hpp>
#include <unistd.h>


using namespace marlin::core;
using namespace marlin::asyncio;

int main() {
	std::string beacon_addr = "0.0.0.0:90002";
	std::string discovery_addr = "0.0.0.0:15002";
	std::string pubsub_addr = "0.0.0.0:15000";

	uint8_t static_sk[crypto_sign_SECRETKEYBYTES];
	uint8_t static_pk[crypto_sign_PUBLICKEYBYTES];
	crypto_sign_keypair(static_pk, static_sk);
	DefaultMulticastClientOptions clop {
		static_sk,
		static_pk,
		std::vector<uint16_t>({0, 1}),
		beacon_addr,
		discovery_addr,
		pubsub_addr
	};

	OnRampNear onrampNear(clop);
	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
