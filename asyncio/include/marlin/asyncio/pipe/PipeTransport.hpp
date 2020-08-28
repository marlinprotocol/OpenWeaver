#ifndef MARLIN_ASYNCIO_PIPETRANSPORT
#define MARLIN_ASYNCIO_PIPETRANSPORT

#include <uv.h>
#include <marlin/core/Buffer.hpp>
#include "marlin/asyncio/core/Timer.hpp"
#include <spdlog/spdlog.h>

namespace marlin {
namespace asyncio {

template<typename DelegateType>
class PipeTransport {
public:
	using SelfType = PipeTransport<DelegateType>;
private:
	uv_pipe_t* pipe = nullptr;

	static void recv_cb(
		uv_stream_t* handle,
		ssize_t nread,
		uv_buf_t const* buf
	);

	static void connect_cb(uv_connect_t* req, int status);
public:
	DelegateType* delegate;

	PipeTransport();
	~PipeTransport();

	void connect(std::string path);
	void send(core::WeakBuffer bytes);
	void close();
};

}  // namespace asyncio
}  // namespace marlin

// Impl
#include "PipeTransport.ipp"

#endif  // MARLIN_ASYNCIO_PIPETRANSPORT
