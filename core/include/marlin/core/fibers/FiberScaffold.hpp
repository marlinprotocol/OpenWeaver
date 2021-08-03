#ifndef MARLIN_CORE_FIBERS_FIBERSCAFFOLD_HPP
#define MARLIN_CORE_FIBERS_FIBERSCAFFOLD_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/SocketAddress.hpp>

namespace marlin {
namespace core {


template<typename Fiber, typename ExtFabric, typename IMT = Buffer, typename OMT = Buffer>
class FiberScaffold {
public:
	static constexpr bool is_inner_open = !std::is_same_v<IMT, void>;
	static constexpr bool is_outer_open = !std::is_same_v<OMT, void>;

	using InnerMessageType = IMT;
	using OuterMessageType = OMT;

protected:
	[[no_unique_address]] ExtFabric ext_fabric;

public:
	template<typename ExtTupleType>
	FiberScaffold(std::tuple<ExtTupleType>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)) {
	}

	int did_recv(auto&&, InnerMessageType&& buf, SocketAddress addr) {
		return ext_fabric.i(*(Fiber*)this).did_recv(ext_fabric.is(*(Fiber*)this), std::move(buf), addr);
	}

	int did_dial(auto&&, SocketAddress addr, auto&&... args) {
		return ext_fabric.i(*(Fiber*)this).did_dial(ext_fabric.is(*(Fiber*)this), addr, std::forward<decltype(args)>(args)...);
	}

	int send(auto&&, InnerMessageType&& buf, auto&&... args) {
		return ext_fabric.o(*(Fiber*)this).send(ext_fabric.os(*(Fiber*)this), std::move(buf), std::forward<decltype(args)>(args)...);
	}

	int did_send(auto&&, InnerMessageType&& buf) {
		return ext_fabric.i(*(Fiber*)this).did_send(ext_fabric.is(*(Fiber*)this), std::move(buf));
	}

	void close() {
		return ext_fabric.o(*(Fiber*)this).close();
	}

	int did_close(auto&&) {
		return ext_fabric.i(*(Fiber*)this).did_close(ext_fabric.is(*(Fiber*)this));
	}

	int did_connect(auto&&, auto&&... args) {
		return ext_fabric.i(*(Fiber*)this).did_connect(ext_fabric.is(*(Fiber*)this), std::forward<decltype(args)>(args)...);
	}

	int did_disconnect(auto&&, auto&&... args) {
		return ext_fabric.i(*(Fiber*)this).did_disconnect(ext_fabric.is(*(Fiber*)this), std::forward<decltype(args)>(args)...);
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_FIBERSCAFFOLD_HPP
