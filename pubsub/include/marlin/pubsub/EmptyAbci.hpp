#ifndef MARLIN_PUBSUB_EMPTY_ABCI
#define MARLIN_PUBSUB_EMPTY_ABCI

#include <marlin/core/Buffer.hpp>

namespace marlin {
namespace pubsub {

template<typename DelegateType, typename...>
class EmptyAbci {
public:
	DelegateType* delegate;

	EmptyAbci(DelegateType* delegate) : delegate(delegate) {
		delegate->did_connect(*this);
	}

	template<typename... MT>
	uint64_t analyze_block(core::Buffer&& block, MT&&... mt) {
		return delegate->did_analyze_block(
			*this,
			std::move(block),
			"",
			"",
			core::WeakBuffer(nullptr, 0),
			std::forward<MT>(mt)...
		);
	}
};

}  // namespace pubsub
}  // namespace marlin

#endif  // MARLIN_PUBSUB_EMPTY_ABCI
