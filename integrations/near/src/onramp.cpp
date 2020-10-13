#include <marlin/near/NearTransportFactory.hpp>
#include <sodium.h>


using namespace marlin::near;
using namespace marlin::core;
using namespace marlin::asyncio;

// struct Delegate {
// 	void did_recv_message(NearTransport <Delegate> &, Buffer &&message) {
// 		SPDLOG_INFO(
// 			"msgSize: {}; Message received from NearTransport: {}",
// 			message.size(),
// 			spdlog::to_hex(message.data(), message.data() + message.size())
// 		);
// 		message.release();
// 	}

// 	void did_send_message(NearTransport<Delegate> &, Buffer &&message) {
// 		SPDLOG_INFO(
// 			"Transport {{ Src: {}, Dst: {} }}: Did send message: {} bytes",
// 			// transport.src_addr.to_string(),
// 			// transport.dst_addr.to_string(),
// 			message.size()
// 		);
// 	}

// 	void did_dial(NearTransport<Delegate> &) {
// 		// (void)transport;
// 		// transport.send(Buffer(new char[10], 10));
// 	}

// 	bool should_accept(SocketAddress const &) {
// 		return true;
// 	}

// 	void did_create_transport(NearTransport<Delegate> &transport) {
// 		transport.setup(this);
// 	}

// 	void did_close(NearTransport<Delegate> &, uint16_t) {}
// };



int main() {

	// Delegate d;
	// NearTransportFactory <Delegate, Delegate> nearFactory;

	// nearFactory.bind(SocketAddress::loopback_ipv4(8000));
	// nearFactory.listen(d);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}