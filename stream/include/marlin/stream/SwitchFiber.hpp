#ifndef MARLIN_ASYNCIO_UDP_STREAMFACTORYFIBER_HPP
#define MARLIN_ASYNCIO_UDP_STREAMFACTORYFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/SocketAddress.hpp>
#include <spdlog/spdlog.h>
#include <marlin/core/fibers/FiberScaffold.hpp>
#include <absl/container/flat_hash_map.h>
#include "StreamFiber.hpp"

using namespace marlin::core;

namespace marlin {
namespace stream {

template <typename ExtFabric>
class SwitchFiber : public FiberScaffold<
	SwitchFiber<ExtFabric>,
	ExtFabric,
	Buffer,
	Buffer
>  {

	using SelfType = SwitchFiber<ExtFabric>;
	using BaseTransport = StreamFiber<SelfType>;

	absl::flat_hash_map<
		SocketAddress,
		BaseTransport*
	> transport_map;

	/// Get transport with a given destination address,
	/// creating one if not found
	template<class... Args>
	BaseTransport *get_or_create(
		SocketAddress const &addr
		// Args&&... args
	) {
		auto res = transport_map.try_emplace(
			addr,
			new BaseTransport(addr)
		);
		return res.first->second;
	}

public:
	using FiberScaffoldType = FiberScaffold<
		SelfType,
		ExtFabric,
		Buffer,
		Buffer
	>;

	using typename FiberScaffoldType::InnerMessageType;
	using typename FiberScaffoldType::OuterMessageType;

	using FiberScaffoldType::FiberScaffoldType;


	int did_recv(
		auto &&,	// id of the Stream
		InnerMessageType&& buf,
		SocketAddress &sock
	) {
		// Process
		auto *dummy = get_or_create(sock);
		SPDLOG_INFO("Received something in stream factory fiber. sock_addr={}, port={}", sock.to_string(), sock.get_port());

		dummy->did_recv(std::move(buf));
		return 0;
	}
};

}
}

#endif