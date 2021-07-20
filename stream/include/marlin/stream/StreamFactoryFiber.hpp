#ifndef MARLIN_ASYNCIO_UDP_STREAMFACTORYFIBER_HPP
#define MARLIN_ASYNCIO_UDP_STREAMFACTORYFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/SocketAddress.hpp>
#include <spdlog/spdlog.h>
#include <marlin/core/fibers/FiberScaffold.hpp>
#include <absl/container/flat_hash_map.h>

using namespace marlin::core;

namespace marlin {
namespace stream {

template <typename ExtFabric>
class StreamFactoryFiber : public FiberScaffold<
	StreamFactoryFiber<ExtFabric>,
	ExtFabric,
	Buffer,
	Buffer
>  {
	absl::flat_hash_map<
		SocketAddress,
		uint16_t
	> transport_map;

	/// Get transport with a given destination address,
	/// creating one if not found
	template<class... Args>
	uint16_t get_or_create(
		SocketAddress const &addr
		// Args&&... args
	) {
		auto res = transport_map.try_emplace(
			addr,
			addr.get_port()
		);
		return res.first->second;
	}

public:
	using SelfType = StreamFactoryFiber<ExtFabric>;
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
		InnerMessageType&&,
		SocketAddress &sock
	) {
		// Process
		uint16_t num = get_or_create(sock);
		SPDLOG_INFO("Received something in stream factory fiber. num={}, sock_addr={}, port={}", num, sock.to_string(), sock.get_port());
		return 0;
	}
};

}
}

#endif