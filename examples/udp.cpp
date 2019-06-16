#include "udp/UdpTransportFactory.hpp"
#include <uv.h>
#include <spdlog/spdlog.h>

using namespace marlin::net;

struct Delegate {
	void did_recv_packet(UdpTransport<Delegate> &transport, Buffer &&packet) {
		SPDLOG_INFO(
			"Transport: {{Src: {}, Dst: {}}}, Did recv packet: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			packet.size()
		);
	}

	void did_send_packet(UdpTransport<Delegate> &transport, Buffer &&packet) {
		SPDLOG_INFO(
			"Transport: {{Src: {}, Dst: {}}}, Did send packet: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			packet.size()
		);
	}

	void did_dial(UdpTransport<Delegate> &transport) {
		transport.send(Buffer(new char[10], 10));
	}

	bool should_accept(SocketAddress const &) {
		return true;
	}

	void did_create_transport(UdpTransport<Delegate> &transport) {
		transport.setup(this);
	}
};

int main() {
	UdpTransportFactory<Delegate, Delegate> s, c;
	s.bind(SocketAddress::loopback_ipv4(8000));

	Delegate d;

	s.listen(d);

	c.bind(SocketAddress::loopback_ipv4(0));
	c.dial(SocketAddress::loopback_ipv4(8000), d);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
