#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#include "marlin/simulator/fibers/SimulationFiber.hpp"
#include "marlin/simulator/network/Network.hpp"
#include "marlin/simulator/core/Simulator.hpp"

using namespace marlin::simulator;
using namespace marlin::core;
using namespace std;

struct Delegate;

using FiberType = SimulationFiber<Delegate, NetworkInterface<Network<NetworkConditioner>>, Simulator>;

struct Delegate {
    Delegate(std::tuple<>) {}

	int did_recv(FiberType& fiber, Buffer&& packet, SocketAddress addr) {
		SPDLOG_INFO(
			"Fiber: {{Src: {}, Dst: {}}}, Did recv packet: {} bytes",
			fiber.addr.to_string(),
			addr.to_string(),
			packet.size()
		);
		if(Simulator::default_instance.current_tick() > 10) {
			// transport.close();
		} else {
			fiber.send(Buffer({0,0,0,0,0,0,0,0,0,0}, 10), addr);
		}

        return 0;
	}

	int did_send(FiberType& fiber, Buffer&& packet) {
		SPDLOG_INFO(
			"Fiber: {{Src: {}, Dst: {}}}, Did send packet: {} bytes",
			fiber.addr.to_string(),
			SocketAddress().to_string(),
			packet.size()
		);

		if(Simulator::default_instance.current_tick() > 9) {
			// transport.close();
		}

        return 0;
	}

	int did_dial(FiberType& fiber, SocketAddress addr) {
		SPDLOG_INFO("Did dial");
		fiber.send(Buffer({0,0,0,0,0,0,0,0,0,0}, 10), addr);

        return 0;
	}

	void did_close(FiberType&, uint16_t) {
		SPDLOG_INFO("Did close");
	}

	auto& i(auto&&) { return *this; }
	auto& o(auto&&) { return *this; }
	auto& is(auto& f) { return f; }
	auto& os(auto& f) { return f; }
};

int main() {
	auto& simulator = Simulator::default_instance;
	NetworkConditioner nc;
	Network<NetworkConditioner> network(nc);
	auto& i1 = network.get_or_create_interface(SocketAddress::from_string("192.168.0.1:0"));
	auto& i2 = network.get_or_create_interface(SocketAddress::from_string("192.168.0.2:0"));

	FiberType s(std::forward_as_tuple(
        std::make_tuple(), i1, simulator
    )), c(std::forward_as_tuple(
        std::make_tuple(), i2, simulator
    ));
	s.bind(SocketAddress::from_string("192.168.0.1:8000"));
	s.listen();

	c.bind(SocketAddress::from_string("192.168.0.2:9000"));
	c.dial(SocketAddress::from_string("192.168.0.1:8000"));

	// c.get_transport(SocketAddress::loopback_ipv4(1234));

	SPDLOG_INFO("Simulation start: {}", !simulator.queue.is_empty());

	simulator.run();

	SPDLOG_INFO("Simulation end: {}", simulator.queue.is_empty());

	return 0;
}
