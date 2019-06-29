#include "tcp/TcpTransportFactory.hpp"
#include <uv.h>
#include <spdlog/spdlog.h>

using namespace marlin::net;

struct Delegate {
	void did_recv_bytes(TcpTransport<Delegate> &transport, Buffer &&packet) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did recv packet: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			packet.size()
		);
	}

	void did_send_bytes(TcpTransport<Delegate> &transport, Buffer &&packet) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did send packet: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			packet.size()
		);
	}

	void did_dial(TcpTransport<Delegate> &transport) {
		transport.send(Buffer(new char[10], 10));
	}

	bool should_accept(SocketAddress const &) {
		return true;
	}

	void did_create_transport(TcpTransport<Delegate> &transport) {
		transport.setup(this);
	}
};

int main() {
	TcpTransportFactory<Delegate, Delegate> s, c;
	s.bind(SocketAddress::loopback_ipv4(8000));

	Delegate d;

	s.listen(d);

	c.bind(SocketAddress::loopback_ipv4(0));
	c.dial(SocketAddress::loopback_ipv4(8000), d);

	c.get_transport(SocketAddress::loopback_ipv4(1234));

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
