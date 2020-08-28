namespace marlin {
namespace asyncio {

//---------------- Helper macros begin ----------------//

#define PIPETRANSPORT_TEMPLATE typename DelegateType

#define PIPETRANSPORT PipeTransport< \
	DelegateType \
>

//---------------- Helper macros end ----------------//

template<PIPETRANSPORT_TEMPLATE>
PIPETRANSPORT::PipeTransport() {
	pipe = new uv_pipe_t();
	pipe->data = this;
}

template<PIPETRANSPORT_TEMPLATE>
PIPETRANSPORT::~PipeTransport() {
	uv_close((uv_handle_t*)pipe, [](uv_handle_t* handle) {
		delete handle;
	});
}

template<PIPETRANSPORT_TEMPLATE>
void PIPETRANSPORT::recv_cb(
	uv_stream_t* handle,
	ssize_t nread,
	uv_buf_t const* buf
) {
	auto* transport = (SelfType*)handle->data;

	// EOF
	if(nread == -4095) {
		transport->delegate->did_disconnect(*transport, 0);
		delete[] buf->base;
		return;
	}

	// Error
	if(nread < 0) {
		SPDLOG_ERROR(
			"PipeTransport: Recv callback error: {}",
			nread
		);

		transport->close();
		delete[] buf->base;
		return;
	}

	if(nread == 0) {
		delete[] buf->base;
		return;
	}

	transport->delegate->did_recv(
		*transport,
		core::Buffer((uint8_t*)buf->base, nread)
	);
}

template<PIPETRANSPORT_TEMPLATE>
void PIPETRANSPORT::connect_cb(uv_connect_t* req, int status) {
	auto* transport = (SelfType*)req->data;
	delete req;

	SPDLOG_INFO("PipeTransport: Status: {}", status);
	if(status < 0) {
		transport->delegate->did_disconnect(*transport, 1);
		return;
	}

	auto res = uv_read_start(
		(uv_stream_t*)transport->pipe,
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
			"PipeTransport: Read start error: {}",
			res
		);
		transport->close();
	}

	transport->delegate->did_connect(*transport);
}

template<PIPETRANSPORT_TEMPLATE>
void PIPETRANSPORT::connect(std::string path) {
	uv_pipe_init(uv_default_loop(), pipe, 0);

	auto req = new uv_connect_t();
	req->data = this;
	uv_pipe_connect(
		req,
		pipe,
		path.c_str(),
		connect_cb
	);
}

template<PIPETRANSPORT_TEMPLATE>
void PIPETRANSPORT::send(core::WeakBuffer bytes) {
	auto* req = new uv_write_t();
	req->data = this;

	auto buf = uv_buf_init((char*)bytes.data(), bytes.size());
	int res = uv_write(
		req,
		(uv_stream_t*)pipe,
		&buf,
		1,
		[](uv_write_t *req, int status) {
			delete req;
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
}

template<PIPETRANSPORT_TEMPLATE>
void PIPETRANSPORT::close() {
	uv_close((uv_handle_t*)pipe, [](uv_handle_t* handle) {
		delete handle;
	});
	delegate->did_close(*this);
}

//---------------- Helper macros undef begin ----------------//

#undef PIPETRANSPORT_TEMPLATE
#undef PIPETRANSPORT

//---------------- Helper macros undef end ----------------//

}  // namespace asyncio
}  // namespace marlin
