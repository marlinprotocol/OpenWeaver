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

	template<typename FiberType>
	int did_dial(FiberType& fabric, SocketAddress addr) {
		SPDLOG_INFO("Terminal: Did dial: {}", addr.to_string());
		fabric.send(Buffer({0,0,0,0,0}, 5), addr);
		return 0;
	}

	template<typename FiberType>
	int did_send(FiberType&, Buffer&& buf) {
		SPDLOG_INFO("Terminal: Did send: {} bytes", buf.size());
		return 0;
	}
};


int main() {
	Fabric<
		Terminal,
		UdpFiber
	> server(std::make_tuple(
		// terminal
		std::make_tuple(),
		// udp fiber
		std::make_tuple()
	));
	server.bind(SocketAddress::from_string("127.0.0.1:8000"));
	server.listen();

	Fabric<
		Terminal,
		UdpFiber
	> client(std::make_tuple(
		// terminal
		std::make_tuple(),
		// udp fiber
		std::make_tuple()
	));
	client.bind(SocketAddress::from_string("127.0.0.1:9000"));
	client.dial(SocketAddress::from_string("127.0.0.1:8000"));

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
