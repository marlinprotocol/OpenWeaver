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

	uint8_t counter = 0;
	uint64_t resp_id = 0;
public:
	DelegateType* delegate;
	core::SocketAddress dst;

	Abci(std::string dst) : connect_timer(this), dst(core::SocketAddress::from_string(dst)) {
		tcp.delegate = this;
		connect_timer_cb();
	}

	// Delegate
	void did_connect(BaseTransport& transport);
	void did_recv(BaseTransport& transport, core::Buffer&& bytes);
	void did_disconnect(BaseTransport& transport, uint reason);
	void did_close(BaseTransport& transport);

	void close() {
		tcp.close();
	}

	void get_block_number();
	template<typename... MT>
	uint64_t analyze_block(core::Buffer&& block, MT&&... metadata);
};

}  // namespace cosmos
}  // namespace marlin

// Impl
#include "Abci.ipp"

#endif  // MARLIN_COSMOS_ABCI
