namespace marlin {
namespace asyncio {

//---------------- Helper macros begin ----------------//

#define TCPTRANSPORTNEAR_TEMPLATE typename DelegateType

#define TCPTRANSPORTNEAR TcpTransportNear< \
	DelegateType \
>

//---------------- Helper macros end ----------------//

template<TCPTRANSPORTNEAR_TEMPLATE>
TCPTRANSPORTNEAR::TcpTransportNear() {
	tcp = new uv_tcp_t();
	tcp->data = this;
}

template<TCPTRANSPORTNEAR_TEMPLATE>
TCPTRANSPORTNEAR::~TcpTransportNear() {
	uv_close((uv_handle_t*)tcp, [](uv_handle_t* handle) {
		delete handle;
	});
}

template<TCPTRANSPORTNEAR_TEMPLATE>
void TCPTRANSPORTNEAR::recv_cb(
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
			"TcpTransportNear: Recv callback error: {}",
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

template<TCPTRANSPORTNEAR_TEMPLATE>
void TCPTRANSPORTNEAR::connect_cb(uv_connect_t* req, int status) {
	auto* transport = (SelfType*)req->data;
	delete req;

	SPDLOG_INFO("TcpTransportNear: Status: {}", status);
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
			"TcpTransportNear: Read start error: {}",
			res
		);
		transport->close();
	}

	transport->delegate->did_connect(*transport);
}

template<TCPTRANSPORTNEAR_TEMPLATE>
void TCPTRANSPORTNEAR::connect(const sockaddr* addr) {
	uv_tcp_init(uv_default_loop(), tcp);

	auto req = new uv_connect_t();
	req->data = this;
	uv_tcp_connect(
		req,
		tcp,
		addr,
		connect_cb
	);
}

template<TCPTRANSPORTNEAR_TEMPLATE>
void TCPTRANSPORTNEAR::send(core::WeakBuffer bytes) {
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

template<TCPTRANSPORTNEAR_TEMPLATE>
void TCPTRANSPORTNEAR::close() {
	uv_close((uv_handle_t*)tcp, [](uv_handle_t* handle) {
		delete handle;
	});
	delegate->did_close(*this);
}

//---------------- Helper macros undef begin ----------------//

#undef TCPTRANSPORTNEAR_TEMPLATE
#undef TCPTRANSPORTNEAR

//---------------- Helper macros undef end ----------------//

}  // namespace asyncio
}  // namespace marlin