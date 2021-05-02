#ifndef MARLIN_CORE_FIBERS_VERSIONINGFIBER_HPP
#define MARLIN_CORE_FIBERS_VERSIONINGFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/transports/VersionedTransport.hpp>

namespace marlin {
namespace core {

template<typename ExtFabric>
class VersioningFiber {
public:
	using SelfType = VersioningFiber<ExtFabric>;

	static constexpr bool is_outer_open = true;
	static constexpr bool is_inner_open = true;

	using OuterMessageType = Buffer;
	using InnerMessageType = VersionedMessage<Buffer>;

private:
	[[no_unique_address]] ExtFabric ext_fabric;

public:
	template<typename ExtTupleType>
	VersioningFiber(std::tuple<ExtTupleType>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)) {
	}

	int did_recv(InnerMessageType&& buf, SocketAddress addr) {
		if(!buf.validate()) {
			// Validation failure
			return -1;
		}
		return ext_fabric.did_recv(*this, std::move(buf), addr);
	}

	int did_dial(SocketAddress addr, auto&&... args) {
		return ext_fabric.did_dial(*this, addr, std::forward<decltype(args)>(args)...);
	}

	int send(InnerMessageType&& buf, SocketAddress addr) {
		return ext_fabric.send(*this, std::move(buf), addr);
	}

	int did_send(InnerMessageType&& buf) {
		return ext_fabric.did_send(*this, std::move(buf));
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_VERSIONINGFIBER_HPP
