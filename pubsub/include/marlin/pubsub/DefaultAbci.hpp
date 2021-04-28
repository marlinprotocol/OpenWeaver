#ifndef MARLIN_PUBSUB_DEFAULT_ABCI
#define MARLIN_PUBSUB_DEFAULT_ABCI

#include <marlin/core/Buffer.hpp>

namespace marlin {
namespace pubsub {

template<typename, typename...>
class DefaultAbci {
public:
	void* delegate;

	template<typename... Args>
	DefaultAbci(Args&&...) {}

	int submit_receipt_onchain(core::Buffer &&) {
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

}  // namespace pubsub
}  // namespace marlin

#endif  // MARLIN_PUBSUB_DEFAULT_ABCI
