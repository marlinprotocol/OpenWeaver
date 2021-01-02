// #include "marlin/asyncio/udp/UdpTransportFactory.hpp"
#include <uv.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <libwebsockets.h>

// using namespace marlin::core;
// using namespace marlin::asyncio;

int status = 0;

int http_handler(lws* conn, lws_callback_reasons reason, void* user, void* in, size_t len) {
	switch (reason) {
	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		SPDLOG_ERROR("lws conn error: {}", in ? (char*)in : "<unknown>");
		break;
	case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
		SPDLOG_INFO("lws conn established");
		char buf[256];
		lws_get_peer_simple(conn, buf, sizeof(buf));
		status = lws_http_client_http_response(conn);

		SPDLOG_INFO("lws conn: {}, http response: {}", buf, status);
		break;
	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
		SPDLOG_INFO("lws read");
		SPDLOG_INFO("lws read: {} bytes: {}", len, std::string((char const*)in, (char const*)in+len));
		return 0; /* don't passthru */
	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
		{
			char buffer[1024 + LWS_PRE];
			char *px = buffer + LWS_PRE;
			int lenx = sizeof(buffer) - LWS_PRE;

			if (lws_http_client_read(conn, &px, &lenx) < 0)
				return -1;
		}
		return 0; /* don't passthru */

	case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
		SPDLOG_INFO("lws conn completed: {}", status);
		lws_cancel_service(lws_get_context(conn)); /* abort poll wait */
		break;

	case LWS_CALLBACK_CLOSED_CLIENT_HTTP:
		SPDLOG_INFO("lws conn closed: {}", status);
		lws_cancel_service(lws_get_context(conn)); /* abort poll wait */
		break;

	default:
		break;
	}

	return lws_callback_http_dummy(conn, reason, user, in, len);
}


int main() {
	lws_context_creation_info info;
	std::memset(&info, 0, sizeof(info));  // prevents some issues with garbage values

	void* loop = uv_default_loop();
	lws_protocols protocols[] = {
		{ "http", http_handler, 0, 0, 0, 0, 0 },
		{ nullptr, nullptr, 0, 0, 0, 0, 0 }
	};

	info.foreign_loops = &loop;
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.options = LWS_SERVER_OPTION_LIBUV | LWS_SERVER_OPTION_UV_NO_SIGSEGV_SIGFPE_SPIN;
	info.connect_timeout_secs = 5;
	info.protocols = protocols;

	auto* context = lws_create_context(&info);
	if(context == nullptr) {
		SPDLOG_ERROR("Failed to init libws context");
		return 1;
	}

	lws_client_connect_info connect_info;
	std::memset(&connect_info, 0, sizeof(connect_info));  // prevents some issues with garbage values
	connect_info.context = context;
	connect_info.address = "example.com";
	connect_info.port = 80;
	connect_info.ssl_connection = 0;
	connect_info.method = "GET";
	connect_info.protocol = "http";
	connect_info.alpn = "http/1.1";
	connect_info.path = "/";
	connect_info.host = connect_info.address;
	connect_info.origin = connect_info.address;

	auto* conn = lws_client_connect_via_info(&connect_info);
	(void)conn;

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

