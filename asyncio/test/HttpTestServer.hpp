#ifndef MARLIN_ASYNCIO_TEST_HTTPTESTSERVER_HPP
#define MARLIN_ASYNCIO_TEST_HTTPTESTSERVER_HPP

#include "gtest/gtest.h"

#include <uv.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

// HTTP server on a loopback port for tests. It answers every request with
// the same status line and body, after delay_ms. The body is body_size
// bytes of 'a' unless one is set with set_reply.
struct TestServer {
	int fd = -1;
	uint16_t port = 0;
	std::mutex mutex;
	std::string status;
	std::string body;
	bool has_body = false;
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

	// Sets the status line and body of the replies from now on
	void set_reply(std::string new_status, std::string new_body) {
		std::lock_guard<std::mutex> lock(mutex);
		status = new_status;
		body = new_body;
		has_body = true;
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

			std::string reply_status, reply_body;
			bool reply_has_body;
			{
				std::lock_guard<std::mutex> lock(mutex);
				reply_status = status;
				reply_body = body;
				reply_has_body = has_body;
			}
			auto size = reply_has_body ? reply_body.size() : body_size;

			std::string head = "HTTP/1.1 " + reply_status + "\r\nContent-Type: application/json\r\nConnection: close\r\n";
			if(send_length) {
				head += "Content-Length: " + std::to_string(size) + "\r\n";
			}
			head += "\r\n";
			(void)!send(conn, head.data(), head.size(), MSG_NOSIGNAL);

			std::string chunk = reply_has_body ? reply_body : std::string(65536, 'a');
			size_t sent = 0;
			while(sent < size) {
				auto n = send(conn, chunk.data(), std::min(chunk.size(), size - sent), MSG_NOSIGNAL);
				if(n <= 0) {
					break;
				}
				sent += n;
			}
			close(conn);
		}
	}
};

// Runs the default loop until done() is true or 5 s have passed. Returns done().
inline bool run_loop_until(std::function<bool()> done) {
	auto start = std::chrono::steady_clock::now();
	while(!done()) {
		if(std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
			return false;
		}
		uv_run(uv_default_loop(), UV_RUN_NOWAIT);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	return true;
}

#endif // MARLIN_ASYNCIO_TEST_HTTPTESTSERVER_HPP
