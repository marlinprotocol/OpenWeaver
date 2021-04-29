#include "marlin/asyncio/udp/UdpFiber.hpp"
#include <marlin/core/fabric/Fabric.hpp>

#include <spdlog/spdlog.h>

using namespace marlin::core;
using namespace marlin::asyncio;

struct Terminal {
	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = false;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	template<typename... Args>
	Terminal(Args&&...) {}

	template<typename FiberType>
	int did_recv(FiberType&, Buffer&& buf, SocketAddress addr) {
		SPDLOG_INFO("Terminal: Did recv: {} bytes from {}", buf.size(), addr.to_string());
		return 0;
	}
};


int main() {
	Fabric<
		Terminal,
		UdpFiber
	> f(std::make_tuple(
		// terminal
		std::make_tuple(std::make_tuple(), 0),
		// udp fiber
		std::make_tuple()
	));
	f.bind(SocketAddress::from_string("127.0.0.1:8000"));
	f.listen();

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
