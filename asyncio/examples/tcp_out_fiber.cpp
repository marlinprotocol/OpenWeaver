#include <marlin/asyncio/tcp/TcpServerFiber.hpp>
#include <marlin/asyncio/tcp/TcpInFiber.hpp>
#include <marlin/asyncio/tcp/TcpOutFiber.hpp>
#include <marlin/core/fibers/MappingFiber.hpp>
#include <uv.h>
#include <spdlog/spdlog.h>

using namespace marlin::core;
using namespace marlin::asyncio;


struct ServerTerminal {
	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = false;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	template<typename... Args>
	ServerTerminal(Args&&...) {}

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "did_recv"_tag) {
			return did_recv(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_dial"_tag) {
			return did_dial(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_send"_tag) {
			return did_send(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_close"_tag) {
			return did_close(std::forward<decltype(args)>(args)...);
		} else {
			static_assert(tag < 0);
		}
	}

private:
	int did_recv(auto&&, Buffer&& buf, SocketAddress addr) {
		SPDLOG_INFO("ServerTerminal: Did recv: {} bytes from {}", buf.size(), addr.to_string());
		return 0;
	}

	template<typename FiberType>
	int did_dial(FiberType& fabric, SocketAddress addr) {
		SPDLOG_INFO("ServerTerminal: Did dial: {}", addr.to_string());
		fabric.template inner_call<"send"_tag>(0, addr, Buffer({0,0,0,0,0}, 5));
		return 0;
	}

	template<typename FiberType>
	int did_send(FiberType&, Buffer&& buf, SocketAddress) {
		SPDLOG_INFO("ServerTerminal: Did send: {} bytes", buf.size());
		return 0;
	}

	int did_close(auto&&, auto&&...) {
		return 0;
	}
};

struct ClientTerminal {
	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = false;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	template<typename... Args>
	ClientTerminal(Args&&...) {}

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "did_recv"_tag) {
			return did_recv(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_dial"_tag) {
			return did_dial(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_send"_tag) {
			return did_send(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_close"_tag) {
			return did_close(std::forward<decltype(args)>(args)...);
		} else {
			static_assert(tag < 0);
		}
	}

private:
	int did_recv(auto&&, Buffer&& buf, SocketAddress addr) {
		SPDLOG_INFO("ClientTerminal: Did recv: {} bytes from {}", buf.size(), addr.to_string());
		return 0;
	}

	template<typename FiberType>
	int did_dial(FiberType& fabric, SocketAddress addr) {
		SPDLOG_INFO("ClientTerminal: Did dial: {}", addr.to_string());
		fabric.template inner_call<"send"_tag>(0, Buffer({0,0,0,0,0}, 5));
		return 0;
	}

	template<typename FiberType>
	int did_send(FiberType&, Buffer&& buf) {
		SPDLOG_INFO("ClientTerminal: Did send: {} bytes", buf.size());
		return 0;
	}

	int did_close(auto&&, auto&&...) {
		return 0;
	}
};


int main() {
	Fabric<
		ServerTerminal,
		TcpServerFiber,
		MappingFiberF<TcpInFiber, SocketAddress>::type
	> server(std::make_tuple(
		// terminal
		std::make_tuple(),
		// udp fiber
		std::make_tuple(),
		std::make_tuple(std::make_tuple())
	));
	(void)server.template outer_call<"bind"_tag>(0, SocketAddress::from_string("127.0.0.1:8000"));
	(void)server.template outer_call<"listen"_tag>(0);

	Fabric<
		ClientTerminal,
		TcpOutFiber
	> client(std::make_tuple(
		// terminal
		std::make_tuple(),
		std::make_tuple()
	));
	// (void)client.template outer_call<"bind"_tag>(0, SocketAddress::from_string("127.0.0.1:9000"));
	(void)client.template outer_call<"dial"_tag>(0, SocketAddress::from_string("127.0.0.1:8000"));

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
