#ifndef MARLIN_ASYNCIO_UDP_UDPFIBER_HPP
#define MARLIN_ASYNCIO_UDP_UDPFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>
#include <marlin/core/messages/BaseMessage.hpp>
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

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "bind"_tag) {
			return bind(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "listen"_tag) {
			return listen(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "dial"_tag) {
			return dial(std::forward<decltype(args)>(args)...);
		} else {
			return FiberScaffoldType::template outer_call<tag>(std::forward<decltype(args)>(args)...);
		}
	}

	template<uint32_t tag>
	auto inner_call(auto&&... args) {
		if constexpr (tag == "send"_tag) {
			return send(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "close"_tag) {
			return close(std::forward<decltype(args)>(args)...);
		} else {
			return FiberScaffoldType::template inner_call<tag>(std::forward<decltype(args)>(args)...);
		}
	}

private:
	[[nodiscard]] int bind(auto&&, core::SocketAddress const& addr) {
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

		fiber.ext_fabric.template outer_call<"did_recv"_tag>(
			fiber,
			core::Buffer((uint8_t*)buf->base, nread),
			addr
		);
	}

	[[nodiscard]] int listen(auto&&) {
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

	[[nodiscard]] int dial(auto&&, core::SocketAddress addr, auto&&... args) {
		// listen if not already
		if(!udp_handle->is_active()) {
			auto status = listen(*this);
			if(status < 0) {
				return status;
			}
		}

		return FiberScaffoldType::template outer_call<"did_dial"_tag>(*this, addr, std::forward<decltype(args)>(args)...);
	}

	static void send_cb(
		uv_udp_send_t* _req,
		int status
	) {
		auto* req = (uvpp::UdpSendReq<std::tuple<core::Buffer, core::SocketAddress>>*)_req;

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
			fiber.ext_fabric.template outer_call<"did_send"_tag>(
				fiber,
				std::move(std::get<0>(req->extra_data)),
				std::get<1>(req->extra_data)
			);
		}

		delete req;
	}

	[[nodiscard]] int send(auto&& src, core::BaseMessage&& buf, core::SocketAddress addr) {
		return send(std::forward<decltype(src)>(src), std::move(buf.buf), addr);
	}

	[[nodiscard]] int send(auto&&, core::Buffer&& buf, core::SocketAddress addr) {
		auto* req = new uvpp::UdpSendReq<std::tuple<core::Buffer, core::SocketAddress>>(std::forward_as_tuple(std::move(buf), addr));
		req->data = this;

		auto uv_buf = uv_buf_init((char*)std::get<0>(req->extra_data).data(), std::get<0>(req->extra_data).size());
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

	void close(auto&&, auto&&... args) {
		this->ext_fabric.template outer_call<"did_close"_tag>(*this, std::forward<decltype(args)>(args)...);
	}
};

}  // namespace asyncio
}  // namespace marlin

#endif  // MARLIN_ASYNCIO_UDP_UDPFIBER_HPP
