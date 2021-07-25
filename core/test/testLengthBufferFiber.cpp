#include <gtest/gtest.h>
#include <marlin/core/fibers/LengthBufferFiber.hpp>

using namespace marlin::core;

struct Terminal {
	Terminal(auto&&...) {}

	auto& i(auto&&) { return *this; }
	auto& o(auto&&) { return *this; }
	auto& is(auto& f) { return f; }
	auto& os(auto& f) { return f; }

	size_t c = 1;

	std::function<int(Buffer&&, SocketAddress)> did_recv_impl;
	int did_recv(auto&& src, Buffer&& buf, SocketAddress addr) {
		auto res = did_recv_impl(std::move(buf), addr);
		src.reset(++c);
		return res;
	}

	void reset(uint64_t) {}
};

TEST(LengthBufferFiber, Constructible) {
	Terminal t;
	LengthBufferFiber<Terminal&> f(std::forward_as_tuple(t));
}

TEST(LengthBufferFiber, MultipleBuffers) {
	Terminal t;

	auto msg1 = Buffer(1).write_unsafe(0, (uint8_t const*)"a", 1);
	auto msg2 = Buffer(2).write_unsafe(0, (uint8_t const*)"bc", 2);
	auto msg3 = Buffer(3).write_unsafe(0, (uint8_t const*)"def", 3);
	auto msg4 = Buffer(4).write_unsafe(0, (uint8_t const*)"ghij", 4);
	auto msg5 = Buffer(5).write_unsafe(0, (uint8_t const*)"klmno", 5);

	LengthBufferFiber<Terminal&> f(std::forward_as_tuple(t));

	size_t calls = 0;
	t.did_recv_impl = [&](Buffer&& buf, SocketAddress addr) {
		EXPECT_LT(calls, 5);
		if(calls == 0) {
			EXPECT_EQ(buf.size(), 1);
			EXPECT_EQ(std::memcmp(buf.data(), "a", 1), 0);
		} else if(calls == 1) {
			EXPECT_EQ(buf.size(), 2);
			EXPECT_EQ(std::memcmp(buf.data(), "bc", 2), 0);
		} else if(calls == 2) {
			EXPECT_EQ(buf.size(), 3);
			EXPECT_EQ(std::memcmp(buf.data(), "def", 3), 0);
		} else if(calls == 3) {
			EXPECT_EQ(buf.size(), 4);
			EXPECT_EQ(std::memcmp(buf.data(), "ghij", 4), 0);
		} else if(calls == 4) {
			EXPECT_EQ(buf.size(), 5);
			EXPECT_EQ(std::memcmp(buf.data(), "klmno", 5), 0);
		}
		EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");
		calls++;

		return 0;
	};

	f.reset(1);
	f.did_recv(0, std::move(msg1), 0, SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv_frame(0, SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv(0, std::move(msg2), 0, SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv_frame(0, SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv(0, std::move(msg3), 0, SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv_frame(0, SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv(0, std::move(msg4), 0, SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv_frame(0, SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv(0, std::move(msg5), 0, SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv_frame(0, SocketAddress::from_string("192.168.0.1:8000"));

	EXPECT_EQ(calls, 5);
}

