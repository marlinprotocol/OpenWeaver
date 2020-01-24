#include "gtest/gtest.h"
#include <marlin/net/udp/UdpTransportFactory.hpp>

#include <functional>

using namespace marlin::net;

struct TransportDelegate {
	std::function<void(UdpTransport<TransportDelegate> &, Buffer &&)> did_recv_packet;
	std::function<void(UdpTransport<TransportDelegate> &, Buffer &&)> did_send_packet;
	std::function<void(UdpTransport<TransportDelegate> &)> did_dial;
};

struct ListenDelegate {
	std::function<bool(SocketAddress const &)> should_accept;
	std::function<void(UdpTransport<TransportDelegate> &)> did_create_transport;
};

TEST(UdpTransport, Constructible) {
	TransportManager<UdpTransport<TransportDelegate>> tm;
	UdpTransport<TransportDelegate> t(
		SocketAddress::loopback_ipv4(8000),
		SocketAddress::loopback_ipv4(8001),
		nullptr,
		tm
	);

	EXPECT_EQ(t.src_addr, SocketAddress::loopback_ipv4(8000));
	EXPECT_EQ(t.dst_addr, SocketAddress::loopback_ipv4(8001));
}

TEST(UdpTransport, CanRecv) {
	TransportManager<UdpTransport<TransportDelegate>> tm;
	UdpTransport<TransportDelegate> t(
		SocketAddress::loopback_ipv4(8000),
		SocketAddress::loopback_ipv4(8001),
		nullptr,
		tm
	);

	bool did_call_delegate = false;

	TransportDelegate td;
	td.did_recv_packet = [&] (
		UdpTransport<TransportDelegate> &transport,
		Buffer &&packet
	) {
		did_call_delegate = true;

		EXPECT_EQ(&transport, &t);
		EXPECT_STREQ(packet.data(), "123456789");
	};

	t.setup(&td);
	t.did_recv_packet(
		Buffer(new char[10] {'1','2','3','4','5','6','7','8','9',0}, 10)
	);

	EXPECT_TRUE(did_call_delegate);
}

void close_cb(uv_handle_t* handle) {
	delete handle;
}

TEST(UdpTransport, CanSend) {
	auto *sock = new uv_udp_t();
	uv_udp_init(uv_default_loop(), sock);

	TransportManager<UdpTransport<TransportDelegate>> tm;
	UdpTransport<TransportDelegate> t(
		SocketAddress::loopback_ipv4(8000),
		SocketAddress::loopback_ipv4(8001),
		sock,
		tm
	);

	bool did_call_delegate = false;

	TransportDelegate td;
	td.did_send_packet = [&] (
		UdpTransport<TransportDelegate> &transport,
		Buffer &&packet
	) {
		did_call_delegate = true;

		EXPECT_EQ(&transport, &t);
		EXPECT_STREQ(packet.data(), "123456789");

		uv_close((uv_handle_t*)sock, close_cb);
	};

	t.setup(&td);
	auto res = t.send(
		Buffer(new char[10] {'1','2','3','4','5','6','7','8','9',0}, 10)
	);

	EXPECT_EQ(res, 0);

	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	EXPECT_TRUE(did_call_delegate);
}

TEST(UdpTransportFactory, CanBind) {
	UdpTransportFactory<ListenDelegate, TransportDelegate> f;

	auto res = f.bind(SocketAddress::loopback_ipv4(8000));

	EXPECT_EQ(res, 0);
}

TEST(UdpTransportFactory, CanListen) {
	UdpTransportFactory<ListenDelegate, TransportDelegate> f;
	f.bind(SocketAddress::loopback_ipv4(8000));

	ListenDelegate delegate;

	auto res = f.listen(delegate);

	EXPECT_EQ(res, 0);
}

TEST(UdpTransportFactory, CanDial) {
	UdpTransportFactory<ListenDelegate, TransportDelegate> f;
	f.bind(SocketAddress::loopback_ipv4(8000));

	bool did_call_f_delegate = false;
	bool did_call_t_delegate = false;

	TransportDelegate td;
	td.did_dial = [&] (UdpTransport<TransportDelegate> &) {
		did_call_t_delegate = true;
	};

	ListenDelegate delegate;
	delegate.did_create_transport = [&] (UdpTransport<TransportDelegate> &t) {
		did_call_f_delegate = true;
		t.delegate = &td;

		EXPECT_EQ(t.src_addr, SocketAddress::loopback_ipv4(8000));
		EXPECT_EQ(t.dst_addr, SocketAddress::loopback_ipv4(8001));
	};

	auto res = f.dial(SocketAddress::loopback_ipv4(8001), delegate);

	EXPECT_EQ(res, 1);
	EXPECT_TRUE(did_call_f_delegate);
	EXPECT_TRUE(did_call_t_delegate);
}
