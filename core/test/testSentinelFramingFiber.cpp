#include <gtest/gtest.h>
#include <marlin/core/fibers/SentinelFramingFiber.hpp>

using namespace marlin::core;

struct Source {
	int leftover(auto&& source, auto&& buf, SocketAddress addr) {
		return source.did_recv(*this, std::move(buf), addr);
	}
};

struct Terminal {
	Terminal(auto&&...) {}

	auto& i(auto&&) { return *this; }
	auto& o(auto&&) { return *this; }
	auto& is(auto& f) { return f; }
	auto& os(auto& f) { return f; }

	std::function<int(Buffer&&, SocketAddress)> did_recv_impl;
	int did_recv(auto&&, Buffer&& buf, SocketAddress addr) {
		return did_recv_impl(std::move(buf), addr);
	}

	std::function<int(SocketAddress)> did_recv_sentinel_impl;
	int did_recv_sentinel(auto&&, SocketAddress addr) {
		return did_recv_sentinel_impl(addr);
	}
};

TEST(SentinelFramingFiber, Constructible) {
	SentinelFramingFiber<Terminal, '\n'> f(std::make_tuple(std::make_tuple()));
}

TEST(SentinelFramingFiber, SingleBufferNoSentinels) {
	Source s;
	Terminal t;

	Buffer msg(5);
	msg.write_unsafe(0, (uint8_t const*)"hello", 5);

	SentinelFramingFiber<Terminal&, '\n'> f(std::forward_as_tuple(t));

	size_t calls = 0;
	t.did_recv_impl = [&](Buffer&& buf, SocketAddress addr) {
		EXPECT_EQ(calls, 0);
		calls++;

		EXPECT_EQ(std::memcmp(buf.data(), "hello", 5), 0);
		EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");

		return 0;
	};
	f.did_recv(s, std::move(msg), SocketAddress::from_string("192.168.0.1:8000"));
	EXPECT_EQ(calls, 1);
}

TEST(SentinelFramingFiber, SingleBufferOneSentinel) {
	Source s;
	Terminal t;

	Buffer msg(11);
	msg.write_unsafe(0, (uint8_t const*)"hello\nworld", 11);

	SentinelFramingFiber<Terminal&, '\n'> f(std::forward_as_tuple(t));

	size_t bytes_calls = 0;
	size_t sentinel_calls = 0;
	t.did_recv_impl = [&](Buffer&& buf, SocketAddress addr) {
		EXPECT_LT(bytes_calls, 2);
		if(bytes_calls == 0) {
			EXPECT_EQ(sentinel_calls, 0);
			EXPECT_EQ(std::memcmp(buf.data(), "hello", 5), 0);
			EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");
		} else if(bytes_calls == 1) {
			EXPECT_EQ(sentinel_calls, 1);
			EXPECT_EQ(std::memcmp(buf.data(), "world", 5), 0);
			EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");
		}
		bytes_calls++;

		return 0;
	};
	t.did_recv_sentinel_impl = [&](SocketAddress addr) {
		EXPECT_EQ(bytes_calls, 1);
		EXPECT_EQ(sentinel_calls, 0);
		sentinel_calls++;
		EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");

		return 0;
	};
	f.did_recv(s, std::move(msg), SocketAddress::from_string("192.168.0.1:8000"));
	EXPECT_EQ(bytes_calls, 2);
	EXPECT_EQ(sentinel_calls, 1);
}

TEST(SentinelFramingFiber, MultipleBufferOneSentinel1) {
	Source s;
	Terminal t;

	Buffer msg1(5);
	msg1.write_unsafe(0, (uint8_t const*)"hello", 5);
	Buffer msg2(11);
	msg2.write_unsafe(0, (uint8_t const*)"world\nagain", 11);

	SentinelFramingFiber<Terminal&, '\n'> f(std::forward_as_tuple(t));

	size_t bytes_calls = 0;
	size_t sentinel_calls = 0;
	t.did_recv_impl = [&](Buffer&& buf, SocketAddress addr) {
		EXPECT_LT(bytes_calls, 3);
		if(bytes_calls == 0) {
			EXPECT_EQ(sentinel_calls, 0);
			EXPECT_EQ(std::memcmp(buf.data(), "hello", 5), 0);
			EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");
		} else if(bytes_calls == 1) {
			EXPECT_EQ(sentinel_calls, 0);
			EXPECT_EQ(std::memcmp(buf.data(), "world", 5), 0);
			EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");
		} else if(bytes_calls == 2) {
			EXPECT_EQ(sentinel_calls, 1);
			EXPECT_EQ(std::memcmp(buf.data(), "again", 5), 0);
			EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");
		}
		bytes_calls++;

		return 0;
	};
	t.did_recv_sentinel_impl = [&](SocketAddress addr) {
		EXPECT_EQ(bytes_calls, 2);
		EXPECT_EQ(sentinel_calls, 0);
		sentinel_calls++;
		EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");

		return 0;
	};

	f.did_recv(s, std::move(msg1), SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv(s, std::move(msg2), SocketAddress::from_string("192.168.0.1:8000"));

	EXPECT_EQ(bytes_calls, 3);
	EXPECT_EQ(sentinel_calls, 1);
}

TEST(SentinelFramingFiber, MultipleBufferOneSentinel2) {
	Source s;
	Terminal t;

	Buffer msg1(5);
	msg1.write_unsafe(0, (uint8_t const*)"hello", 5);
	Buffer msg2(1);
	msg2.write_unsafe(0, (uint8_t const*)"\n", 1);
	Buffer msg3(5);
	msg3.write_unsafe(0, (uint8_t const*)"world", 5);

	SentinelFramingFiber<Terminal&, '\n'> f(std::forward_as_tuple(t));

	size_t bytes_calls = 0;
	size_t sentinel_calls = 0;
	t.did_recv_impl = [&](Buffer&& buf, SocketAddress addr) {
		EXPECT_LT(bytes_calls, 3);
		if(bytes_calls == 0) {
			EXPECT_EQ(sentinel_calls, 0);
			EXPECT_EQ(std::memcmp(buf.data(), "hello", 5), 0);
			EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");
		} else if(bytes_calls == 1) {
			EXPECT_EQ(sentinel_calls, 0);
			EXPECT_EQ(std::memcmp(buf.data(), "\n", 1), 0);
			EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");
		} else if(bytes_calls == 2) {
			EXPECT_EQ(sentinel_calls, 1);
			EXPECT_EQ(std::memcmp(buf.data(), "world", 5), 0);
			EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");
		}
		bytes_calls++;

		return 0;
	};
	t.did_recv_sentinel_impl = [&](SocketAddress addr) {
		EXPECT_EQ(bytes_calls, 2);
		EXPECT_EQ(sentinel_calls, 0);
		sentinel_calls++;
		EXPECT_EQ(addr.to_string(), "192.168.0.1:8000");

		return 0;
	};

	f.did_recv(s, std::move(msg1), SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv(s, std::move(msg2), SocketAddress::from_string("192.168.0.1:8000"));
	f.did_recv(s, std::move(msg3), SocketAddress::from_string("192.168.0.1:8000"));

	EXPECT_EQ(bytes_calls, 3);
	EXPECT_EQ(sentinel_calls, 1);
}

