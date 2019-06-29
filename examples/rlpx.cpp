#include "RlpxTransportFactory.hpp"

using namespace marlin::rlpx;
using namespace marlin::net;

struct Delegate {
	void did_recv_message(RlpxTransport<Delegate> &transport, Buffer &&message) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did recv message: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			message.size()
		);

		if(message.data()[0] == 0x10) { // eth63 Status
			transport.send(std::move(message));
		} else if(message.data()[0] == 0x13) { // eth63 GetBlockHeaders
			transport.send(Buffer(new char[2]{0x14, (char)0xc0}, 2));
		} else if(message.data()[0] == 0x15) { // eth63 GetBlockBodies
			transport.send(Buffer(new char[2]{0x16, (char)0xc0}, 2));
		}
	}

	void did_send_message(RlpxTransport<Delegate> &transport, Buffer &&message) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did send message: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			message.size()
		);
	}

	void did_dial(RlpxTransport<Delegate> &transport) {
		(void)transport;
		// transport.send(Buffer(new char[10], 10));
	}

	bool should_accept(SocketAddress const &) {
		return true;
	}

	void did_create_transport(RlpxTransport<Delegate> &transport) {
		transport.setup(this);
	}
};

int main() {
	Delegate d;
	RlpxTransportFactory<Delegate, Delegate> f;

	f.bind(SocketAddress::loopback_ipv4(8000));
	f.listen(d);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
