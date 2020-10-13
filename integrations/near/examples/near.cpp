#include <marlin/near/OnRampNear.hpp>
#include <unistd.h>


using namespace marlin::core;
using namespace marlin::asyncio;

int main() {

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);

	OnRampNear onrampNear;
	std::cout << "LOL\n";
	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
