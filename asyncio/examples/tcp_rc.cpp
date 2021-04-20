#include "marlin/asyncio/tcp/ReconnectingTcpTransportFactory.hpp"
#include <uv.h>
#include <spdlog/spdlog.h>

using namespace marlin::core;
using namespace marlin::asyncio;

struct Delegate {
	void did_recv(ReconnectingTcpTransport<Delegate> &transport, Buffer &&bytes) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did recv bytes: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			bytes.size()
		);
	}

	void did_send(ReconnectingTcpTransport<Delegate> &transport, Buffer &&bytes) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did send bytes: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			bytes.size()
		);
		transport.close();
	}

	void did_dial(ReconnectingTcpTransport<Delegate> &transport) {
		transport.send(Buffer({0,0,0,0,0,0,0,0,0,0}, 10));
	}

	bool should_accept(SocketAddress const &) {
		return true;
	}

	void did_create_transport(ReconnectingTcpTransport<Delegate> &transport) {
		transport.setup(this);
	}

	void did_close(ReconnectingTcpTransport<Delegate> &, uint16_t) {}
};

int main() {
	ReconnectingTcpTransportFactory<Delegate, Delegate> s, c;
	s.bind(SocketAddress::loopback_ipv4(8000));

	Delegate d;

	s.listen(d);

	c.bind(SocketAddress::loopback_ipv4(0));
	c.dial(SocketAddress::loopback_ipv4(8000), d);

	c.get_transport(SocketAddress::loopback_ipv4(1234));

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
