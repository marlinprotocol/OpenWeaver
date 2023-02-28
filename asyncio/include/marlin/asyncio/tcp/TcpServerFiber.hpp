#ifndef MARLIN_ASYNCIO_TCP_TCPSERVERFIBER_HPP
#define MARLIN_ASYNCIO_TCP_TCPSERVERFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>
#include <marlin/core/fabric/Fabric.hpp>
#include <marlin/core/fibers/VersioningFiber.hpp>

#include <marlin/uvpp/Tcp.hpp>

#include <uv.h>
#include <spdlog/spdlog.h>


namespace marlin {
namespace asyncio {

template<typename ExtFabric>
class TcpServerFiber : public core::FiberScaffold<
	TcpServerFiber<ExtFabric>,
	ExtFabric,
	core::Buffer,
	void
> {
public:
	using SelfType = TcpServerFiber<ExtFabric>;
	using FiberScaffoldType = core::FiberScaffold<
		SelfType,
		ExtFabric,
		core::Buffer,
		void
	>;

	using typename FiberScaffoldType::InnerMessageType;
	using typename FiberScaffoldType::OuterMessageType;

private:
	uv_tcp_t* tcp_handle = nullptr;
	core::SocketAddress addr;

public:
	TcpServerFiber(auto&&... args) :
		FiberScaffoldType(std::forward<decltype(args)>(args)...) {
		tcp_handle = new uv_tcp_t();
		tcp_handle->data = this;
	}

	TcpServerFiber(TcpServerFiber const&) = delete;
	TcpServerFiber(TcpServerFiber&&) = delete;

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "bind"_tag) {
			return bind(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "listen"_tag) {
			return listen(std::forward<decltype(args)>(args)...);
		} else {
			return FiberScaffoldType::template outer_call<tag>(std::forward<decltype(args)>(args)...);
		}
	}

	template<uint32_t tag>
	auto inner_call(auto&&... args) {
		if constexpr (tag == "close"_tag) {
			return close(std::forward<decltype(args)>(args)...);
		} else {
			static_assert(tag < 0);
		}
	}

private:
	[[nodiscard]] int bind(auto&&, core::SocketAddress const& addr) {
		this->addr = addr;

		int res = uv_tcp_init(uv_default_loop(), tcp_handle);
		if (res < 0) {
			SPDLOG_ERROR(
				"TcpServerFiber: Socket {}: Init error: {}",
				this->addr.to_string(),
				res
			);
			return res;
		}

		res = uv_tcp_bind(
			tcp_handle,
			reinterpret_cast<sockaddr const*>(&this->addr),
			0
		);
		if (res < 0) {
			SPDLOG_ERROR(
				"TcpServerFiber: Socket {}: Bind error: {}",
				this->addr.to_string(),
				res
			);
			return res;
		}

		return 0;
	}

	// connection callback on new connections
	static void connection_cb(uv_stream_t* handle, int status) {
		auto& fiber = *(SelfType*)(handle->data);

		if (status < 0) {
			SPDLOG_ERROR(
				"TcpServerFiber: Socket {}: Connection callback error: {}",
				fiber.addr.to_string(),
				status
			);

			return;
		}

		// create handle
		auto* client = new uv_tcp_t();
		status = uv_tcp_init(uv_default_loop(), client);
		if (status < 0) {
			SPDLOG_ERROR(
				"TcpServerFiber: Socket {}: TCP init error: {}",
				fiber.addr.to_string(),
				status
			);

			delete client;

			return;
		}

		// accept incoming conn
		status = uv_accept(handle, (uv_stream_t*)client);
		if (status < 0) {
			SPDLOG_ERROR(
				"TcpServerFiber: Socket {}: Connection accept error: {}",
				fiber.addr.to_string(),
				status
			);

			delete client;

			return;
		}

		// get remote addr
		sockaddr saddr;
		int len = sizeof(sockaddr);

		status = uv_tcp_getpeername(client, &saddr, &len);
		if (status < 0) {
			SPDLOG_ERROR(
				"TcpServerFiber: Socket {}: Getpeername error: {}",
				fiber.addr.to_string(),
				status
			);

			delete client;

			return;
		}

		auto& addr = *reinterpret_cast<core::SocketAddress const*>(&saddr);

		fiber.template outer_call<"did_dial"_tag>(fiber, addr, client);
	}

	[[nodiscard]] int listen(auto&&) {
		int res = uv_listen((uv_stream_t*)tcp_handle, 100, connection_cb);
		if (res < 0) {
			SPDLOG_ERROR(
				"TcpServerFiber: Socket {}: Listen error: {}",
				this->addr.to_string(),
				res
			);
			return res;
		}

		return 0;
	}

	void close(auto&&) {
		uv_close((uv_handle_t*)tcp_handle, [](uv_handle_t* handle) {
			auto& fiber = *(SelfType*)handle->data;
			fiber.template outer_call<"did_close"_tag>(fiber);
			delete handle;
		});
	}
};


}  // namespace asyncio
}  // namespace marlin

#endif  // MARLIN_ASYNCIO_TCP_TCPSERVERFIBER_HPP
