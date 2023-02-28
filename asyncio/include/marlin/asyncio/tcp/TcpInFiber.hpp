#ifndef MARLIN_ASYNCIO_TCP_TCPINFIBER_HPP
#define MARLIN_ASYNCIO_TCP_TCPINFIBER_HPP

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
class TcpInFiber : public core::FiberScaffold<
	TcpInFiber<ExtFabric>,
	ExtFabric,
	core::Buffer,
	void
> {
public:
	using SelfType = TcpInFiber<ExtFabric>;
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
	TcpInFiber(auto&& init_tuple) :
		FiberScaffoldType(std::forward_as_tuple(std::get<0>(init_tuple))) {}

	TcpInFiber(TcpInFiber const&) = delete;
	TcpInFiber(TcpInFiber&&) = delete;

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "did_dial"_tag) {
			return did_dial(std::forward<decltype(args)>(args)...);
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
				"TcpInFiber: Recv callback error: {}",
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

		fiber.template outer_call<"did_recv"_tag>(
			fiber,
			fiber.dst,
			core::Buffer((uint8_t*)buf->base, nread)
		);
	}

	int did_dial(auto&&, uv_tcp_t* handle, core::SocketAddress dst) {
		handle->data = this;
		this->tcp_handle = handle;
		this->dst = dst;

		auto res = uv_read_start(
			(uv_stream_t*)handle,
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
				"TcpInFiber: Read start error: {}",
				res
			);
			close();
		}

		return FiberScaffoldType::template outer_call<"did_dial"_tag>(*this, dst);
	}

	[[nodiscard]] int send(auto&&, InnerMessageType&& bytes, core::SocketAddress) {
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
				InnerMessageType bytes = std::move(((WriteReqType*)req)->extra_data);
				auto& fiber = *(SelfType*)req->data;
				delete (WriteReqType*)req;
				if(status < 0) {
					SPDLOG_ERROR(
						"TcpInFiber: Send callback error: {}",
						status
					);
					return;
				}

				fiber.template outer_call<"did_send"_tag>(fiber, fiber.dst, std::move(bytes));
			}
		);

		if (res < 0) {
			SPDLOG_ERROR(
				"TcpInFiber: Send error: {}",
				res
			);
			this->close();

			return -1;
		}

		return 0;
	}

	void close() {
		uv_close((uv_handle_t*)tcp_handle, [](uv_handle_t* handle) {
			auto& fiber = *(SelfType*)handle->data;
			fiber.template outer_call<"did_close"_tag>(fiber, fiber.dst);
			delete handle;
		});
	}
};

}  // namespace asyncio
}  // namespace marlin

#endif  // MARLIN_ASYNCIO_TCP_TCPINFIBER_HPP
