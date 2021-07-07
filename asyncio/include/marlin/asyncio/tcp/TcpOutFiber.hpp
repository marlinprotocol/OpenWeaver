#ifndef MARLIN_ASYNCIO_TCP_TCPOUTFIBER_HPP
#define MARLIN_ASYNCIO_TCP_TCPOUTFIBER_HPP

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
class TcpOutFiber : public core::FiberScaffold<
	TcpOutFiber<ExtFabric>,
	ExtFabric,
	core::Buffer,
	void
> {
public:
	using SelfType = TcpOutFiber<ExtFabric>;
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
	core::SocketAddress dst;

public:
	TcpOutFiber(auto&&... args) :
		FiberScaffoldType(std::forward<decltype(args)>(args)...) {
		tcp_handle = new uv_tcp_t();
		tcp_handle->data = this;

		uv_tcp_init(uv_default_loop(), tcp_handle);
	}

	TcpOutFiber(TcpOutFiber const&) = delete;
	TcpOutFiber(TcpOutFiber&&) = delete;

	static void recv_cb(
		uv_stream_t* handle,
		ssize_t nread,
		uv_buf_t const* buf
	) {
		auto& fiber = *(SelfType*)handle->data;

		// EOF
		if(nread == -4095) {
			fiber.close();
			delete[] buf->base;
			return;
		}

		// Error
		if(nread < 0) {
			SPDLOG_ERROR(
				"TcpOutFiber: Recv callback error: {}",
				nread
			);

			fiber.close();
			delete[] buf->base;
			return;
		}

		if(nread == 0) {
			delete[] buf->base;
			return;
		}

		fiber.did_recv(
			fiber,
			core::Buffer((uint8_t*)buf->base, nread),
			fiber.dst
		);
	}

	static void dial_cb(uv_connect_t* req, int status) {
		auto& fiber = *(SelfType*)req->data;
		delete req;

		SPDLOG_INFO("TcpOutFiber: Status: {}", status);
		if(status < 0) {
			fiber.close();
			return;
		}

		auto res = uv_read_start(
			(uv_stream_t*)fiber.tcp_handle,
			[](
				uv_handle_t*,
				size_t suggested_size,
				uv_buf_t* buf
			) {
				buf->base = new char[suggested_size];
				buf->len = suggested_size;
			},
			recv_cb
		);

		if (res < 0) {
			SPDLOG_ERROR(
				"TcpOutFiber: Read start error: {}",
				res
			);
			fiber.close();
		}

		fiber.did_dial(fiber, fiber.dst);
	}

	[[nodiscard]] int dial(core::SocketAddress dst) {
		if(this->dst != core::SocketAddress()) {
			return -1;
		}

		this->dst = dst;

		auto req = new uv_connect_t();
		req->data = this;
		uv_tcp_connect(
			req,
			tcp_handle,
			reinterpret_cast<sockaddr const *>(&this->dst),
			dial_cb
		);

		return 0;
	}

	[[nodiscard]] int send(auto&&, InnerMessageType&& bytes) {
		using WriteReqType = uvpp::WriteReq<InnerMessageType>;

		auto* req = new WriteReqType(std::move(bytes));
		req->data = this;

		auto buf = uv_buf_init((char*)req->extra_data.data(), req->extra_data.size());
		int res = uv_write(
			req,
			(uv_stream_t*)tcp_handle,
			&buf,
			1,
			[](uv_write_t* req, int status) {
				delete (WriteReqType*)req;
				if(status < 0) {
					SPDLOG_ERROR(
						"Abci: Send callback error: {}",
						status
					);
				}
			}
		);

		if (res < 0) {
			SPDLOG_ERROR(
				"Abci: Send error: {}",
				res
			);
			this->close();
		}

		return 0;
	}

	void close() {
		uv_close((uv_handle_t*)tcp_handle, [](uv_handle_t* handle) {
			delete handle;
		});
		// delegate->did_close(*this);
	}
};

}  // namespace asyncio
}  // namespace marlin

#endif  // MARLIN_ASYNCIO_TCP_TCPOUTFIBER_HPP
