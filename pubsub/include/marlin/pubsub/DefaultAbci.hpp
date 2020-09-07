#ifndef MARLIN_BSC_ABCI
#define MARLIN_BSC_ABCI

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <marlin/core/Buffer.hpp>
#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/pipe/PipeTransport.hpp>

namespace marlin {
namespace pubsub {

template<typename DelegateType>
class DefaultAbci {
public:
	using SelfType = DefaultAbci<DelegateType>;
private:
	using BaseTransport = asyncio::PipeTransport<SelfType>;
	void connect_timer_cb() {}
public:
	DelegateType* delegate;
	std::string path;

	DefaultAbci() {}
	DefaultAbci(std::string) {
	}

	// Delegate
	void did_connect(BaseTransport&) {}
	void did_recv(BaseTransport&, core::Buffer&&) {}
	void did_disconnect(BaseTransport&, uint) {}
	void did_close(BaseTransport&) {}

	void close() {}
	void get_block_number() {}
	uint64_t analyze_block(core::Buffer&&) {return 0;}

	// TODO: Remove the following functions before merge.
	core::WeakBuffer get_header(core::WeakBuffer bytes) {
		// TODO: Extract header from 'bytes'.
		return bytes;
	}

	bool check_reward_worthy(core::WeakBuffer) {
		// TODO: return true if the sender who sent 'bytes' should be rewarded .
		return true;
	}

	int submit_receipt_onchain(core::Buffer &&) {
		// TODO: The sender submits the receipt on chain
		return 1;
	}

	uint8_t* get_key() {
		return nullptr;
	}
};

}  // namespace bsc
}  // namespace marlin

#endif  // MARLIN_BSC_ABCI
