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

template<typename DelegateType, typename... MetadataTypes>
class Abci {
public:
	using SelfType = Abci<DelegateType, MetadataTypes...>;
private:
	using BaseTransport = asyncio::PipeTransport<SelfType>;
	BaseTransport pipe;

	uint64_t connect_timer_interval = 1000;
	asyncio::Timer connect_timer;

	void connect_timer_cb() {
		pipe.connect(path);
	}

	uint64_t id = 0;
	std::unordered_map<uint64_t, std::tuple<core::Buffer, MetadataTypes...>> block_store;
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

	void close() {
		pipe.close();
	}

	void get_block_number();
	template<typename... MT>
	uint64_t analyze_block(core::Buffer&& block, MT&&... metadata);
};

}  // namespace bsc
}  // namespace marlin

// Impl
#include "Abci.ipp"

#endif  // MARLIN_BSC_ABCI
