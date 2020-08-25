#ifndef MARLIN_BSC_ABCI
#define MARLIN_BSC_ABCI

#include <uv.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <marlin/core/Buffer.hpp>

namespace marlin {
namespace bsc {

template<typename DelegateType>
class Abci {
public:
	using SelfType = Abci<DelegateType>;
private:
	uv_pipe_t* pipe;

	static void recv_cb(
		uv_stream_t* handle,
		ssize_t nread,
		uv_buf_t const* buf
	) {
		auto* abci = (SelfType*)handle->data;

		// EOF
		if(nread == -4095) {
			abci->close();
			delete[] buf->base;
			return;
		}

		// Error
		if(nread < 0) {
			SPDLOG_ERROR(
				"Abci: Recv callback error: {}",
				nread
			);

			delete[] buf->base;
			return;
		}

		if(nread == 0) {
			delete[] buf->base;
			return;
		}

		abci->delegate->did_recv(
			*abci,
			core::Buffer((uint8_t*)buf->base, nread)
		);
	}

	static void connect_cb(uv_connect_t* req, int status) {
		if(status < 0) {
			SPDLOG_ERROR("Abci connection error: {}", status);
			return;
		}

		auto* abci = (SelfType*)req->data;
		delete req;
		abci->delegate->did_connect(*abci);

		auto res = uv_read_start(
			(uv_stream_t*)abci->pipe,
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
				"Abci: Read start error: {}",
				res
			);
		}
	}

	void connect_timer_cb() {
		connect_timer_interval *= 2;
		if(connect_timer_interval > 64000) {
			connect_timer_interval = 64000;
		}

		auto req = new uv_connect_t();
		req->data = this;
		uv_pipe_connect(
			req,
			pipe,
			(datadir + "/geth.ipc").c_str(),
			connect_cb
		);
	}
public:
	DelegateType* delegate;

	Abci(std::string datadir) : connect_timer(this) {
		pipe = new uv_pipe_t();
		pipe->data = this;
		uv_pipe_init(uv_default_loop(), pipe, 0);

		connect_timer_cb();
	}

	void get_block_number() {
		std::string rpc = "{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[],\"id\":0}";

		auto* req = new uv_write_t();
		req->data = this;

		auto buf = uv_buf_init(rpc.data(), rpc.size());
		int res = uv_write(
			req,
			(uv_stream_t*)pipe,
			&buf,
			1,
			[](uv_write_t *req, int status) {
				if(status < 0) {
					SPDLOG_ERROR(
						"Abci: Send callback error: {}",
						status
					);
				}

				delete req;
			}
		);

		if (res < 0) {
			SPDLOG_ERROR(
				"Abci: Send error: {}",
				res
			);
		}
	}

	void close() {
		uv_close((uv_handle_t*)pipe, [](uv_handle_t* handle) {
			delete handle;
		});
		delegate->did_close(*this);
	}
};

}  // namespace bsc
}  // namespace marlin

#endif  // MARLIN_BSC_ABCI
