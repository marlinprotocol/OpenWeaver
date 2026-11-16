#include "gtest/gtest.h"
#include "marlin/asyncio/http/HttpPost.hpp"

#include "HttpTestServer.hpp"

#include <chrono>
#include <string>
#include <thread>

using namespace marlin::asyncio;

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

TEST(HttpPost, RejectsOtherProtocols) {
	for(std::string url : {"file:///etc/hostname", "ftp://127.0.0.1/", "gopher://127.0.0.1/", "dict://127.0.0.1/"}) {
		int calls = 0;
		HttpResponse res;
		http_post(url, "{}", [&](HttpResponse&& r) {
			calls++;
			res = std::move(r);
		});
		uv_run(uv_default_loop(), UV_RUN_DEFAULT);

		EXPECT_EQ(calls, 1) << url;
		EXPECT_NE(res.error, "") << url;
		EXPECT_TRUE(res.body.empty()) << url;
	}
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

TEST(HttpPost, CancelledRequestDoesNotCallBack) {
	TestServer server("200 OK", 1000, true, 300);

	// Stands for an object which is destroyed while its request is in flight
	auto* calls = new int(0);
	auto handle = http_post(server.url(), "{}", [calls](HttpResponse&&) {
		(*calls)++;
	});

	// Let the request reach the server, which holds its reply back
	while(server.num_requests == 0) {
		uv_run(uv_default_loop(), UV_RUN_NOWAIT);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	EXPECT_EQ(*calls, 0);

	handle.cancel();
	int* freed = calls;
	calls = nullptr;

	auto start = std::chrono::steady_clock::now();
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	auto elapsed = std::chrono::steady_clock::now() - start;

	EXPECT_EQ(*freed, 0);
	delete freed;
	// The transfer is aborted instead of running into its 30 s timeout
	EXPECT_LT(elapsed, std::chrono::seconds(5));
}

TEST(HttpPost, CancelBeforeTheRequestStarts) {
	TestServer server("200 OK", 1000);

	int calls = 0;
	auto handle = http_post(server.url(), "{}", [&](HttpResponse&&) {
		calls++;
	});
	handle.cancel();
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	EXPECT_EQ(calls, 0);
}

TEST(HttpPost, CancelAfterCompletionDoesNothing) {
	TestServer server("200 OK", 1000);

	int calls = 0;
	auto handle = http_post(server.url(), "{}", [&](HttpResponse&&) {
		calls++;
	});
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	EXPECT_EQ(calls, 1);

	handle.cancel();
	handle.cancel();
	HttpPostHandle().cancel();
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	EXPECT_EQ(calls, 1);
}

TEST(HttpPost, RetryDelay) {
	// Grapher interval
	EXPECT_EQ(subgraph_retry_delay(1, 300000), 15000u);
	EXPECT_EQ(subgraph_retry_delay(2, 300000), 30000u);
	EXPECT_EQ(subgraph_retry_delay(3, 300000), 60000u);
	EXPECT_EQ(subgraph_retry_delay(4, 300000), 120000u);
	EXPECT_EQ(subgraph_retry_delay(5, 300000), 240000u);
	EXPECT_EQ(subgraph_retry_delay(6, 300000), 300000u);
	EXPECT_EQ(subgraph_retry_delay(7, 300000), 300000u);
	EXPECT_EQ(subgraph_retry_delay(UINT64_MAX, 300000), 300000u);
	// StakeRequester interval
	EXPECT_EQ(subgraph_retry_delay(1, 60000), 15000u);
	EXPECT_EQ(subgraph_retry_delay(2, 60000), 30000u);
	EXPECT_EQ(subgraph_retry_delay(3, 60000), 60000u);
	EXPECT_EQ(subgraph_retry_delay(1000, 60000), 60000u);
	// Never longer than the interval
	EXPECT_EQ(subgraph_retry_delay(0, 300000), 15000u);
	EXPECT_EQ(subgraph_retry_delay(1, 1000), 1000u);
}
