#include "gtest/gtest.h"
#include "marlin/asyncio/http/HttpPost.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

using namespace marlin::asyncio;

namespace {

// HTTP server on a loopback port which answers every request with the
// same status line and a body of body_size bytes, after delay_ms.
struct TestServer {
	int fd = -1;
	uint16_t port = 0;
	std::string status;
	size_t body_size;
	bool send_length;
	int delay_ms;
	std::atomic<int> num_requests{0};
	std::thread thread;

	TestServer(
		std::string status,
		size_t body_size,
		bool send_length = true,
		int delay_ms = 0
	) : status(status), body_size(body_size), send_length(send_length), delay_ms(delay_ms) {
		fd = socket(AF_INET, SOCK_STREAM, 0);
		EXPECT_GE(fd, 0);

		sockaddr_in addr = {};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = 0;
		EXPECT_EQ(bind(fd, (sockaddr*)&addr, sizeof(addr)), 0);
		EXPECT_EQ(listen(fd, 16), 0);

		socklen_t len = sizeof(addr);
		EXPECT_EQ(getsockname(fd, (sockaddr*)&addr, &len), 0);
		port = ntohs(addr.sin_port);

		thread = std::thread([this]() { serve(); });
	}

	~TestServer() {
		// Unblocks accept
		shutdown(fd, SHUT_RDWR);
		thread.join();
		close(fd);
	}

	std::string url() const {
		return "http://127.0.0.1:" + std::to_string(port) + "/subgraphs/name/test";
	}

	void serve() {
		while(true) {
			int conn = accept(fd, nullptr, nullptr);
			if(conn < 0) {
				return;
			}
			num_requests++;

			// Read the request up to the end of the headers, the body is short and not needed
			std::string req;
			char buf[4096];
			while(req.find("\r\n\r\n") == std::string::npos) {
				auto n = recv(conn, buf, sizeof(buf), 0);
				if(n <= 0) {
					break;
				}
				req.append(buf, n);
			}

			if(delay_ms > 0) {
				std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
			}

			std::string head = "HTTP/1.1 " + status + "\r\nContent-Type: application/json\r\nConnection: close\r\n";
			if(send_length) {
				head += "Content-Length: " + std::to_string(body_size) + "\r\n";
			}
			head += "\r\n";
			(void)!send(conn, head.data(), head.size(), MSG_NOSIGNAL);

			std::string chunk(65536, 'a');
			size_t sent = 0;
			while(sent < body_size) {
				auto n = send(conn, chunk.data(), std::min(chunk.size(), body_size - sent), MSG_NOSIGNAL);
				if(n <= 0) {
					break;
				}
				sent += n;
			}
			close(conn);
		}
	}
};

}

TEST(HttpPost, ReturnsStatusAndBody) {
	TestServer server("200 OK", 1000);

	int calls = 0;
	HttpResponse res;
	http_post(server.url(), "{}", [&](HttpResponse&& r) {
		calls++;
		res = std::move(r);
	});
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	EXPECT_EQ(calls, 1);
	EXPECT_EQ(res.error, "");
	EXPECT_EQ(res.status, 200);
	EXPECT_EQ(res.body, std::string(1000, 'a'));
}

TEST(HttpPost, ReturnsHttpErrors) {
	TestServer server("500 Internal Server Error", 3);

	int calls = 0;
	HttpResponse res;
	http_post(server.url(), "{}", [&](HttpResponse&& r) {
		calls++;
		res = std::move(r);
	});
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	EXPECT_EQ(calls, 1);
	EXPECT_EQ(res.error, "");
	EXPECT_EQ(res.status, 500);
	EXPECT_EQ(res.body, "aaa");
}

TEST(HttpPost, ReportsConnectionErrors) {
	uint16_t port;
	{
		// A port nothing listens on any more
		TestServer server("200 OK", 0);
		port = server.port;
	}

	int calls = 0;
	HttpResponse res;
	http_post("http://127.0.0.1:" + std::to_string(port) + "/", "{}", [&](HttpResponse&& r) {
		calls++;
		res = std::move(r);
	});
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	EXPECT_EQ(calls, 1);
	EXPECT_NE(res.error, "");
}

TEST(HttpPost, AcceptsResponseOfMaxSize) {
	TestServer server("200 OK", http_post_max_response_size, false);

	HttpResponse res;
	http_post(server.url(), "{}", [&](HttpResponse&& r) {
		res = std::move(r);
	});
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	EXPECT_EQ(res.error, "");
	EXPECT_EQ(res.status, 200);
	EXPECT_EQ(res.body.size(), http_post_max_response_size);
}

TEST(HttpPost, RejectsLargerResponseWithLength) {
	TestServer server("200 OK", http_post_max_response_size + 1);

	int calls = 0;
	HttpResponse res;
	http_post(server.url(), "{}", [&](HttpResponse&& r) {
		calls++;
		res = std::move(r);
	});
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	EXPECT_EQ(calls, 1);
	EXPECT_NE(res.error.find("larger than"), std::string::npos) << res.error;
	EXPECT_TRUE(res.body.empty());
}

TEST(HttpPost, RejectsLargerResponseWithoutLength) {
	// No Content-Length, the body ends when the server closes the connection
	TestServer server("200 OK", 4 * http_post_max_response_size, false);

	int calls = 0;
	HttpResponse res;
	http_post(server.url(), "{}", [&](HttpResponse&& r) {
		calls++;
		res = std::move(r);
	});
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	EXPECT_EQ(calls, 1);
	EXPECT_NE(res.error.find("larger than"), std::string::npos) << res.error;
	EXPECT_TRUE(res.body.empty());
}
