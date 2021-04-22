namespace marlin {
namespace asyncio {

//---------------- Helper macros begin ----------------//

#define RCTCPTRANSPORT_TEMPLATE typename DelegateType

#define RCTCPTRANSPORT RcTcpTransport< \
	DelegateType \
>

//---------------- Helper macros end ----------------//

template<RCTCPTRANSPORT_TEMPLATE>
RCTCPTRANSPORT::RcTcpTransport() {
	tcp = new uv_tcp_t();
	tcp->data = this;
}

template<RCTCPTRANSPORT_TEMPLATE>
RCTCPTRANSPORT::~RcTcpTransport() {
	uv_close((uv_handle_t*)tcp, [](uv_handle_t* handle) {
		delete handle;
	});
}

template<RCTCPTRANSPORT_TEMPLATE>
void RCTCPTRANSPORT::recv_cb(
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
			"RcTcpTransport: Recv callback error: {}",
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

template<RCTCPTRANSPORT_TEMPLATE>
void RCTCPTRANSPORT::connect_cb(uv_connect_t* req, int status) {
	auto* transport = (SelfType*)req->data;
	delete req;

	SPDLOG_INFO("RcTcpTransport: Status: {}", status);
	if(status < 0) {
		transport->delegate->did_disconnect(*transport, 1);
		return;
	}

	auto res = uv_read_start(
		(uv_stream_t*)transport->tcp,
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
			"RcTcpTransport: Read start error: {}",
			res
		);
		transport->close();
	}

	transport->delegate->did_connect(*transport);
}

template<RCTCPTRANSPORT_TEMPLATE>
void RCTCPTRANSPORT::connect(core::SocketAddress dst) {
	uv_tcp_init(uv_default_loop(), tcp);

	auto req = new uv_connect_t();
	req->data = this;
	uv_tcp_connect(
		req,
		tcp,
		reinterpret_cast<sockaddr const *>(&dst),
		connect_cb
	);
}

template<RCTCPTRANSPORT_TEMPLATE>
void RCTCPTRANSPORT::send(core::WeakBuffer bytes) {
	auto* req = new uv_write_t();
	req->data = this;

	auto buf = uv_buf_init((char*)bytes.data(), bytes.size());
	int res = uv_write(
		req,
		(uv_stream_t*)tcp,
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

template<RCTCPTRANSPORT_TEMPLATE>
void RCTCPTRANSPORT::close() {
	uv_close((uv_handle_t*)tcp, [](uv_handle_t* handle) {
		delete handle;
	});
	delegate->did_close(*this);
}

//---------------- Helper macros undef begin ----------------//

#undef RCTCPTRANSPORT_TEMPLATE
#undef RCTCPTRANSPORT

//---------------- Helper macros undef end ----------------//

}  // namespace asyncio
}  // namespace marlin
