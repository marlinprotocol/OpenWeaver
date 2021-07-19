#include "marlin/asyncio/tcp/TcpTransportFactory.hpp"
#include "marlin/asyncio/tcp/TcpOutFiber.hpp"
#include <uv.h>
#include <spdlog/spdlog.h>

using namespace marlin::core;
using namespace marlin::asyncio;

struct Delegate {
	void did_recv(TcpTransport<Delegate> &transport, Buffer &&bytes) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did recv bytes: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			bytes.size()
		);
	}

	void did_send(TcpTransport<Delegate> &transport, Buffer &&bytes) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did send bytes: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			bytes.size()
		);
		transport.close();
	}

	void did_dial(TcpTransport<Delegate> &transport) {
		transport.send(Buffer({0,0,0,0,0,0,0,0,0,0}, 10));
	}

	bool should_accept(SocketAddress const &) {
		return true;
	}

	void did_create_transport(TcpTransport<Delegate> &transport) {
		transport.setup(this);
	}

	void did_close(TcpTransport<Delegate> &, uint16_t) {}
};


struct Terminal {
	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = false;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	template<typename... Args>
	Terminal(Args&&...) {}

	int did_recv(auto&&, Buffer&& buf, SocketAddress addr) {
		SPDLOG_INFO("Terminal: Did recv: {} bytes from {}", buf.size(), addr.to_string());
		return 0;
	}

	template<typename FiberType>
	int did_dial(FiberType& fabric, SocketAddress addr) {
		SPDLOG_INFO("Terminal: Did dial: {}", addr.to_string());
		fabric.o(*this).send(0, Buffer({0,0,0,0,0}, 5));
		return 0;
	}

	template<typename FiberType>
	int did_send(FiberType&, Buffer&& buf) {
		SPDLOG_INFO("Terminal: Did send: {} bytes", buf.size());
		return 0;
	}

	int did_close(auto&&) {
		return 0;
	}
};


int main() {
	TcpTransportFactory<Delegate, Delegate> s;
	s.bind(SocketAddress::loopback_ipv4(8000));

	Delegate d;

	s.listen(d);

	Fabric<
		Terminal,
		TcpOutFiber,
		VersioningFiber
	> client(std::make_tuple(
		// terminal
		std::make_tuple(),
		// udp fiber
		std::make_tuple(),
		std::make_tuple()
	));
	// (void)client.i(client).bind(SocketAddress::from_string("127.0.0.1:9000"));
	(void)client.i(client).dial(SocketAddress::from_string("127.0.0.1:8000"));

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
