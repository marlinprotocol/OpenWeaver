#ifndef MARLIN_ASYNCIO_TCPTRANSPORTNEAR
#define MARLIN_ASYNCIO_TCPTRANSPORTNEAR

#include <uv.h>
#include <marlin/core/Buffer.hpp>
#include "marlin/asyncio/core/Timer.hpp"
#include <spdlog/spdlog.h>

namespace marlin {
namespace asyncio {

template<typename DelegateType>
class TcpTransportNear {
public:
	using SelfType = TcpTransportNear<DelegateType>;
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

	TcpTransportNear();
	TcpTransportNear(TcpTransportNear const&) = delete;
	~TcpTransportNear();

	void connect(const sockaddr* addr);
	void send(core::WeakBuffer bytes);
	void close();
};

}  // namespace asyncio
}  // namespace marlin

// Impl
#include "TcpTransportNear.ipp"

#endif  // MARLIN_ASYNCIO_TCPTRANSPORTNEAR
