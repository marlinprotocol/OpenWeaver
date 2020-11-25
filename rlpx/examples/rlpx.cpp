#include <marlin/rlpx/RlpxTransportFactory.hpp>

using namespace marlin::rlpx;
using namespace marlin::core;
using namespace marlin::asyncio;

struct Delegate {
	void did_recv(RlpxTransport<Delegate> &transport, Buffer &&message) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did recv message: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			message.size()
		);

		if(message.data()[0] == 0x10) { // eth63 Status
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: Status message: {} bytes",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				message.size()
			);
			transport.send(std::move(message));
		} else if(message.data()[0] == 0x13) { // eth63 GetBlockHeaders
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: GetBlockHeaders message: {} bytes",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				message.size()
			);
			transport.send(Buffer({0x14, 0xc0}, 2));
		} else if(message.data()[0] == 0x15) { // eth63 GetBlockBodies
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: GetBlockBodies message: {} bytes",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				message.size()
			);
			transport.send(Buffer({0x16, 0xc0}, 2));
		} else if(message.data()[0] == 0x12) { // eth63 Transactions
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: Transactions message: {} bytes",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				message.size()
			);
		} else if(message.data()[0] == 0x17) { // eth63 NewBlock
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: NewBlock message: {} bytes",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				message.size()
			);
		} else {
			SPDLOG_INFO(
				"Transport {{ Src: {}, Dst: {} }}: Unknown message: {} bytes: {}",
				transport.src_addr.to_string(),
				transport.dst_addr.to_string(),
				spdlog::to_hex(message.data(), message.data() + message.size())
			);
		}
	}

	void did_send(RlpxTransport<Delegate> &transport, Buffer &&message) {
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

	void did_close(RlpxTransport<Delegate> &, uint16_t) {}
};

int main() {
	Delegate d;
	RlpxTransportFactory<Delegate, Delegate> f;

	f.bind(SocketAddress::loopback_ipv4(8000));
	f.listen(d);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
