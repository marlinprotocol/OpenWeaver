#ifndef MARLIN_ASYNCIO_TCP_RCTCPTRANSPORT
#define MARLIN_ASYNCIO_TCP_RCTCPTRANSPORT

#include <uv.h>
#include <marlin/core/Buffer.hpp>
#include <marlin/core/SocketAddress.hpp>
#include "marlin/asyncio/core/Timer.hpp"
#include <spdlog/spdlog.h>

namespace marlin {
namespace asyncio {

template<typename DelegateType>
class RcTcpTransport {
public:
	using SelfType = RcTcpTransport<DelegateType>;
private:
	uv_tcp_t* tcp = nullptr;

	static void recv_cb(
		uv_stream_t* handle,
		ssize_t nread,
		uv_buf_t const* buf
	);

	static void connect_cb(uv_connect_t* req, int status);
public:
	DelegateType* delegate;

	RcTcpTransport();
	RcTcpTransport(RcTcpTransport const&) = delete;
	~RcTcpTransport();

	void connect(core::SocketAddress dst);
	void send(core::WeakBuffer bytes);
	void close();
};

}  // namespace asyncio
}  // namespace marlin

// Impl
#include "RcTcpTransport.ipp"

#endif  // MARLIN_ASYNCIO_TCP_RCTCPTRANSPORT
