#ifndef MARLIN_ASYNCIO_TCP_RCTCPOUTFIBER_HPP
#define MARLIN_ASYNCIO_TCP_RCTCPOUTFIBER_HPP

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
class RcTcpOutFiber : public core::FiberScaffold<
	RcTcpOutFiber<ExtFabric>,
	ExtFabric,
	core::Buffer,
	void
> {
public:
	using SelfType = RcTcpOutFiber<ExtFabric>;
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
	RcTcpOutFiber(auto&&... args) :
		FiberScaffoldType(std::forward<decltype(args)>(args)...) {
		tcp_handle = new uv_tcp_t();
		tcp_handle->data = this;
	}

	RcTcpOutFiber(RcTcpOutFiber const&) = delete;
	RcTcpOutFiber(RcTcpOutFiber&&) = delete;

	static void recv_cb(
		uv_stream_t* handle,
		ssize_t nread,
		uv_buf_t const* buf
	) {
		auto& fiber = *(SelfType*)handle->data;

		// EOF
		if(nread == -4095) {
			fiber.did_disconnect(fiber, 0);
			delete[] buf->base;
			return;
		}

		// Error
		if(nread < 0) {
			SPDLOG_ERROR(
				"RcTcpOutFiber: Recv callback error: {}",
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

		SPDLOG_DEBUG("RcTcpOutFiber: Status: {}", status);
		if(status < 0) {
			fiber.did_disconnect(fiber, 1);
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
				"RcTcpOutFiber: Read start error: {}",
				res
			);
			fiber.close();
		}

		fiber.did_connect(fiber);
	}

	[[nodiscard]] int dial(core::SocketAddress dst) {
		uv_tcp_init(uv_default_loop(), tcp_handle);

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
			auto& fiber = *(SelfType*)handle->data;
			fiber.did_close(fiber);
			delete handle;
		});
	}
};

}  // namespace asyncio
}  // namespace marlin

#endif  // MARLIN_ASYNCIO_TCP_RCTCPOUTFIBER_HPP
