#ifndef MARLIN_ASYNCIO_UDP_UDPFIBER_HPP
#define MARLIN_ASYNCIO_UDP_UDPFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>
#include <marlin/uvpp/Udp.hpp>

#include <spdlog/spdlog.h>

namespace marlin {
namespace asyncio {

template<typename ExtFabric>
class UdpFiber : public core::FiberScaffold<
	UdpFiber<ExtFabric>,
	ExtFabric,
	core::Buffer,
	void
> {
public:
	using SelfType = UdpFiber<ExtFabric>;
	using FiberScaffoldType = core::FiberScaffold<
		SelfType,
		ExtFabric,
		core::Buffer,
		void
	>;

	using typename FiberScaffoldType::InnerMessageType;
	using typename FiberScaffoldType::OuterMessageType;

private:
	uvpp::UdpE* udp_handle = nullptr;

public:
	UdpFiber(auto&&... args) :
		FiberScaffoldType(std::forward<decltype(args)>(args)...) {
		udp_handle = new uvpp::UdpE();
		udp_handle->data = this;
	}

	UdpFiber(UdpFiber const&) = delete;
	UdpFiber(UdpFiber&&) = delete;

	[[nodiscard]] int bind(core::SocketAddress const& addr) {
		int res = uv_udp_init(uv_default_loop(), udp_handle);
		if (res < 0) {
			SPDLOG_ERROR(
				"Asyncio: Socket {}: Init error: {}",
				addr.to_string(),
				res
			);
			return res;
		}

		res = uv_udp_bind(
			udp_handle,
			reinterpret_cast<sockaddr const*>(&addr),
			0
		);
		if (res < 0) {
			SPDLOG_ERROR(
				"Asyncio: Socket {}: Bind error: {}",
				addr.to_string(),
				res
			);
			return res;
		}

		return 0;
	}

	static void naive_alloc_cb(
		uv_handle_t*,
		size_t suggested_size,
		uv_buf_t* buf
	) {
		buf->base = new char[suggested_size];
		buf->len = suggested_size;
	}

	static void recv_cb(
		uv_udp_t* handle,
		ssize_t nread,
		uv_buf_t const* buf,
		sockaddr const* _addr,
		unsigned
	) {
		// Error
		if(nread < 0) {
			sockaddr saddr;
			int len = sizeof(sockaddr);

			uv_udp_getsockname(handle, &saddr, &len);

			SPDLOG_ERROR(
				"Asyncio: Socket {}: Recv callback error: {}",
				reinterpret_cast<core::SocketAddress const*>(&saddr)->to_string(),
				nread
			);

			delete[] buf->base;
			return;
		}

		if(nread == 0) {
			delete[] buf->base;
			return;
		}

		auto& addr = *reinterpret_cast<core::SocketAddress const*>(_addr);
		auto& fiber = *(SelfType*)(handle->data);

		fiber.did_recv(
			fiber,
			core::Buffer((uint8_t*)buf->base, nread),
			addr
		);
	}

	[[nodiscard]] int listen() {
		int res = uv_udp_recv_start(
			udp_handle,
			naive_alloc_cb,
			recv_cb
		);
		if (res < 0) {
			SPDLOG_ERROR(
				"Asyncio: Start recv error: {}",
				res
			);
			return res;
		}

		return 0;
	}

	[[nodiscard]] int dial(core::SocketAddress addr, auto&&... args) {
		// listen if not already
		if(!udp_handle->is_active()) {
			auto status = listen();
			if(status < 0) {
				return status;
			}
		}

		FiberScaffoldType::did_dial(*this, addr, std::forward<decltype(args)>(args)...);

		return 0;
	}

	static void send_cb(
		uv_udp_send_t* _req,
		int status
	) {
		auto* req = (uvpp::UdpSendReq<core::Buffer>*)_req;

		if(req->data == nullptr) {
			delete req;
			return;
		}
		auto& fiber = *(UdpFiber*)req->data;

		if(status < 0) {
			SPDLOG_ERROR(
				"Asyncio: Socket: Send callback error: {}",
				status
			);
		} else {
			fiber.did_send(
				fiber,
				std::move(req->extra_data)
			);
		}

		delete req;
	}

	[[nodiscard]] int send(auto&&, core::Buffer&& buf, core::SocketAddress addr) {
		auto* req = new uvpp::UdpSendReq<core::Buffer>(std::move(buf));
		req->data = this;

		auto uv_buf = uv_buf_init((char*)req->extra_data.data(), req->extra_data.size());
		int res = uv_udp_send(
			req,
			udp_handle,
			&uv_buf,
			1,
			reinterpret_cast<const sockaddr*>(&addr),
			send_cb
		);

		if (res < 0) {
			SPDLOG_ERROR(
				"Asyncio: Socket: Send error: {}, To: {}",
				res,
				addr.to_string()
			);
			return res;
		}

		return 0;
	}
};

}  // namespace asyncio
}  // namespace marlin

#endif  // MARLIN_ASYNCIO_UDP_UDPFIBER_HPP
