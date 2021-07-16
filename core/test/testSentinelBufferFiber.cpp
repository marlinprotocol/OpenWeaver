#include <gtest/gtest.h>
#include <marlin/core/fibers/SentinelBufferFiber.hpp>

using namespace marlin::core;

struct Terminal {
	Terminal(auto&&...) {}

	auto& i(auto&&) { return *this; }
	auto& o(auto&&) { return *this; }
	auto& is(auto& f) { return f; }
	auto& os(auto& f) { return f; }

	std::function<int(Buffer&&, SocketAddress)> did_recv_impl;
	int did_recv(auto&& src, Buffer&& buf, SocketAddress addr) {
		auto res = did_recv_impl(std::move(buf), addr);
		src.reset(100);
		return res;
	}
};

TEST(SentinelBufferFiber, Constructible) {
	Terminal t;
	SentinelBufferFiber<Terminal&> f(std::forward_as_tuple(t));
}

TEST(SentinelBufferFiber, MultipleBuffers) {
	Terminal t;

	Buffer msg1(5);
	msg1.write_unsafe(0, (uint8_t const*)"hello", 5);
	Buffer msg2(6);
	msg2.write_unsafe(0, (uint8_t const*)"world\n", 6);
	Buffer msg3(1);
	msg3.write_unsafe(0, (uint8_t const*)"\n", 1);
	Buffer msg4(5);
	msg4.write_unsafe(0, (uint8_t const*)"lorem", 5);

	SentinelBufferFiber<Terminal&> f(std::forward_as_tuple(t));

	size_t calls = 0;
	t.did_recv_impl = [&](Buffer&& buf, SocketAddress addr) {
		EXPECT_LT(calls, 2);
		if(calls == 0) {
			EXPECT_EQ(buf.size(), 11);
			EXPECT_EQ(std::memcmp(buf.data(), "helloworld\n", 11), 0);
			EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");
		} else if(calls == 1) {
			EXPECT_EQ(buf.size(), 1);
			EXPECT_EQ(std::memcmp(buf.data(), "\n", 1), 0);
			EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");
		}
		calls++;

		return 0;
	};

	f.reset(100);
	f.did_recv(0, std::move(msg1), SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv(0, std::move(msg2), SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv_sentinel(0, SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv(0, std::move(msg3), SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv_sentinel(0, SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv(0, std::move(msg4), SocketAddress::from_string("192.168.0.1:8000"));

	EXPECT_EQ(calls, 2);
}

