#include <marlin/lpf/LpfTransportFactory.hpp>
#include <spdlog/spdlog.h>

#include <marlin/asyncio/tcp/TcpTransportFactory.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>

using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::lpf;

template<typename Delegate>
using TransportType = LpfTransport<Delegate, TcpTransport, false, 8>;

template<typename ListenDelegate, typename TransportDelegate>
using TransportFactoryType = LpfTransportFactory<ListenDelegate, TransportDelegate, TcpTransportFactory, TcpTransport, false, 8>;

struct Delegate {
	int did_recv_message(TransportType<Delegate> &transport, Buffer &&message) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did recv message: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			message.size()
		);
		return 0;
	}

	void did_send_message(TransportType<Delegate> &transport, Buffer &&message) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did send message: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			message.size()
		);
		transport.close();
	}

	void did_dial(TransportType<Delegate> &transport) {
		transport.send(Buffer({0,0,0,0,0,0,0,0,0,0}, 10));
	}

	bool should_accept(SocketAddress const &) {
		return true;
	}

	void did_create_transport(TransportType<Delegate> &transport) {
		transport.setup(this);
	}

	void did_close(TransportType<Delegate> &) {
		SPDLOG_INFO("Did close");
	}

	// void cut_through_recv_start(TransportType<Delegate> &, uint8_t, uint64_t) {}
	// void cut_through_recv_bytes(TransportType<Delegate> &, uint8_t, Buffer) {}
	// void cut_through_recv_end(TransportType<Delegate> &, uint8_t) {}
	// void cut_through_recv_flush(TransportType<Delegate> &, uint8_t) {}
};

int main() {
	TransportFactoryType<Delegate, Delegate> s, c;
	s.bind(SocketAddress::loopback_ipv4(8000));

	Delegate d;

	s.listen(d);

	c.bind(SocketAddress::loopback_ipv4(0));
	c.dial(SocketAddress::loopback_ipv4(8000), d);

	c.get_transport(SocketAddress::loopback_ipv4(1234));

	return marlin::asyncio::EventLoop::run();
}
