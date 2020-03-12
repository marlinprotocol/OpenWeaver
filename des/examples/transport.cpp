#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#include "marlin/simulator/transport/SimulatedTransportFactory.hpp"
#include "marlin/simulator/network/Network.hpp"
#include "marlin/simulator/core/Simulator.hpp"

using namespace marlin::simulator;
using namespace marlin::net;
using namespace std;

struct Delegate;

using TransportType = SimulatedTransport<Simulator, NetworkInterface<Network<NetworkConditioner>>, Delegate>;

struct Delegate {
	void did_recv_packet(TransportType& transport, Buffer&& packet) {
		SPDLOG_INFO(
			"Transport: {{Src: {}, Dst: {}}}, Did recv packet: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			packet.size()
		);
		transport.close();
	}

	void did_send_packet(TransportType& transport, Buffer&& packet) {
		SPDLOG_INFO(
			"Transport: {{Src: {}, Dst: {}}}, Did send packet: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			packet.size()
		);
		transport.close();
	}

	void did_dial(TransportType& transport) {
		SPDLOG_INFO("Did dial");
		transport.send(Buffer(new char[10] {0,0,0,0,0,0,0,0,0,0}, 10));
	}

	void did_close(TransportType&) {}

	bool should_accept(SocketAddress const&) {
		return true;
	}

	void did_create_transport(TransportType& transport) {
		transport.setup(this);
	}
};

int main() {
	Simulator simulator;
	NetworkConditioner nc;
	Network<NetworkConditioner> network(nc);
	auto& i1 = network.get_or_create_interface(SocketAddress::from_string("192.168.0.1:0"));
	auto& i2 = network.get_or_create_interface(SocketAddress::from_string("192.168.0.2:0"));

	TransportManager<TransportType> manager;
	SimulatedTransportFactory<Simulator, NetworkInterface<Network<NetworkConditioner>>, Delegate, Delegate> s(i1, simulator), c(i2, simulator);
	s.bind(SocketAddress::from_string("192.168.0.1:8000"));

	Delegate d;

	s.listen(d);

	c.bind(SocketAddress::from_string("192.168.0.2:9000"));
	c.dial(SocketAddress::from_string("192.168.0.1:8000"), d);

	// c.get_transport(SocketAddress::loopback_ipv4(1234));

	SPDLOG_INFO("Simulation start: {}", !simulator.queue.is_empty());

	simulator.run();

	SPDLOG_INFO("Simulation end: {}", simulator.queue.is_empty());

	return 0;
}
