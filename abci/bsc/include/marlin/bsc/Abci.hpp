#ifndef MARLIN_BSC_ABCI
#define MARLIN_BSC_ABCI

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <marlin/core/Buffer.hpp>
#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/pipe/PipeTransport.hpp>

namespace marlin {
namespace bsc {

template<typename DelegateType>
class Abci {
public:
	using SelfType = Abci<DelegateType>;
private:
	using BaseTransport = asyncio::PipeTransport<SelfType>;
	BaseTransport pipe;

	uint64_t connect_timer_interval = 1000;
	asyncio::Timer connect_timer;

	void connect_timer_cb() {
		pipe.connect(path);
	}

	uint64_t id = 0;
	std::unordered_map<uint64_t, core::Buffer> block_store;
public:
	DelegateType* delegate;
	std::string path;

	Abci(std::string datadir) : connect_timer(this), path(datadir + "/geth.ipc") {
		pipe.delegate = this;
		connect_timer_cb();
	}

	// Delegate
	void did_connect(BaseTransport& transport);
	void did_recv(BaseTransport& transport, core::Buffer&& bytes);
	void did_disconnect(BaseTransport& transport, uint reason);
	void did_close(BaseTransport& transport);

	void get_block_number() {
		std::string rpc = "{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[],\"id\":0}";

		pipe.send(core::WeakBuffer((uint8_t*)rpc.data(), rpc.size()));
	}

	void close() {
		pipe.close();
	}

	uint64_t analyze_block(core::Buffer&& block) {
		std::string hex_block("0x");
		hex_block.reserve(2 + block.size()*2);

		for(size_t i = 0; i < block.size(); i++) {
			hex_block += fmt::format("{:02x}", block.data()[i]);
		}

		std::string rpc = fmt::format("{{\"jsonrpc\":\"2.0\",\"method\":\"lin_analyzeBlock\",\"id\":{},\"params\":[\"{}\"]}}", id, hex_block);

		pipe.send(core::WeakBuffer((uint8_t*)rpc.data(), rpc.size()));

		block_store.emplace(id, std::move(block));
		return id++;
	}
};

}  // namespace bsc
}  // namespace marlin

// Impl
#include "Abci.ipp"

#endif  // MARLIN_BSC_ABCI
