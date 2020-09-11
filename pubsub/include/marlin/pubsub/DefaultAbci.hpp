#ifndef MARLIN_DEFAULT_ABCI
#define MARLIN_DEFAULT_ABCI

#include <marlin/core/Buffer.hpp>
#include <marlin/asyncio/core/Timer.hpp>

namespace marlin {
namespace pubsub {

template<typename DelegateType>
class DefaultAbci {
public:
	DelegateType* delegate;

	uint64_t analyze_block(core::Buffer&&) {
		return 0;
	}

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

#endif  // MARLIN_DEFAULT_ABCI
