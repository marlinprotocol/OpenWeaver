#ifndef MARLIN_DEFAULT_ABCI
#define MARLIN_DEFAULT_ABCI

#include <marlin/core/Buffer.hpp>
#include <marlin/asyncio/core/Timer.hpp>

namespace marlin {
namespace pubsub {

template<typename DelegateType, typename... MetadataTypes>
class DefaultAbci {
public:
	DelegateType* delegate;

	int submit_receipt_onchain(core::Buffer &&) {
		// TODO: The sender submits the receipt on chain
		return 1;
	}

	uint8_t* get_key() {
		return nullptr;
	}

	template<typename... MT>
	uint64_t analyze_block(core::Buffer&&, MT&&...) {
		return 0;
	}
};

}  // namespace bsc
}  // namespace marlin

#endif  // MARLIN_DEFAULT_ABCI
