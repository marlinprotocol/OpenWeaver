#ifndef MARLIN_COSMOS_ABCI
#define MARLIN_COSMOS_ABCI

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <marlin/core/Buffer.hpp>
#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/tcp/RcTcpTransport.hpp>

#include <cryptopp/osrng.h>
#include <secp256k1_recovery.h>
#include <boost/filesystem.hpp>
#include <fstream>

namespace marlin {
namespace cosmos {

template<typename DelegateType, typename... MetadataTypes>
class Abci {
public:
	using SelfType = Abci<DelegateType, MetadataTypes...>;
private:
	using BaseTransport = asyncio::RcTcpTransport<SelfType>;
	BaseTransport tcp;

	uint64_t connect_timer_interval = 1000;
	asyncio::Timer connect_timer;

	void connect_timer_cb() {
		tcp.connect(dst);
	}

	uint64_t id = 0;
	std::unordered_map<uint64_t, std::tuple<core::Buffer, MetadataTypes...>> block_store;
	uint8_t key[32];
public:
	DelegateType* delegate;
	core::SocketAddress dst;

	Abci(core::SocketAddress dst) : connect_timer(this), dst(dst) {
		tcp.delegate = this;
		connect_timer_cb();

		if(boost::filesystem::exists("./.marlin/keys/abci/cosmos")) {
			// Load existing keypair
			std::ifstream sk("./.marlin/keys/abci/cosmos", std::ios::binary);
			if(!sk.read((char*)key, 32)) {
				throw;
			}
		} else {
			// Generate a valid key pair
			auto* ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
			do {
				CryptoPP::OS_GenerateRandomBlock(false, key, 32);
			} while(
				secp256k1_ec_seckey_verify(ctx, key) != 1
			);
			secp256k1_context_destroy(ctx);

			// Store for reuse
			boost::filesystem::create_directories("./.marlin/keys/abci");
			std::ofstream sk("./.marlin/keys/abci/cosmos", std::ios::binary);

			sk.write((char*)key, 32);
		}
	}

	// Delegate
	void did_connect(BaseTransport& transport);
	void did_recv(BaseTransport& transport, core::Buffer&& bytes);
	void did_disconnect(BaseTransport& transport, uint reason);
	void did_close(BaseTransport& transport);

	void close() {
		tcp.close();
	}

	uint8_t* get_key() { return key; }

	void get_block_number();
	template<typename... MT>
	uint64_t analyze_block(core::Buffer&& block, MT&&... metadata);
};

}  // namespace cosmos
}  // namespace marlin

// Impl
#include "Abci.ipp"

#endif  // MARLIN_COSMOS_ABCI
