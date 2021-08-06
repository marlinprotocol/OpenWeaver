#ifndef MARLIN_ASYNCIO_UDP_SWITCHFIBER_HPP
#define MARLIN_ASYNCIO_UDP_SWITCHFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/SocketAddress.hpp>
#include <spdlog/spdlog.h>
#include <marlin/core/fibers/FiberScaffold.hpp>
#include <absl/container/flat_hash_map.h>
#include "StreamFiber.hpp"
#include "EmptyTerminal.hpp"

using namespace marlin::core;

namespace marlin {
namespace stream {

template <typename ExtFabric, template<typename...> typename STFabric>
class SwitchFiber : public FiberScaffold<
	SwitchFiber<ExtFabric, STFabric>,
	ExtFabric,
	Buffer,
	Buffer
>  {

	using SelfType = SwitchFiber<ExtFabric, STFabric>;
	using STFabricType = STFabric<EmptyTerminal>;


	absl::flat_hash_map<
		SocketAddress,
		STFabricType*
	> transport_map;

	/// Get transport with a given destination address,
	/// creating one if not found
	template<class... Args>
	STFabricType *get_or_create(
		SocketAddress const &addr
		// Args&&... args
	) {
		auto res = transport_map.try_emplace(
			addr,
			new STFabricType(std::make_tuple(
				std::make_tuple(),
				std::make_tuple(this)
			))
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

	SwitchFiber() {}
	SwitchFiber(SocketAddress) {}

	int did_recv(
		auto &&,	// id of the Stream
		InnerMessageType&& buf,
		SocketAddress &sock
	) {
		// Process
		auto *dummy = get_or_create(sock);
		SPDLOG_INFO("Received something in stream factory fiber. sock_addr={}, port={}", sock.to_string(), sock.get_port());

		dummy->i(0).did_recv(std::move(buf));
		return 0;
	}
};

}
}

#endif