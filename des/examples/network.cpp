#include <iostream>
#include <vector>
#include "marlin/simulator/core/Simulator.hpp"
#include "marlin/simulator/network/NetworkInterface.hpp"
#include "marlin/simulator/network/Network.hpp"

using namespace marlin::simulator;
using namespace marlin::net;
using namespace std;

template<typename NetworkInterfaceType>
struct PingPongNode final : public NetworkListener<NetworkInterfaceType> {
	Simulator& simulator;

	PingPongNode(Simulator& simulator) : simulator(simulator) {}

	void did_recv(
		NetworkInterfaceType& interface,
		uint16_t port,
		SocketAddress const& addr,
		Buffer&& message
	) override {
		SocketAddress taddr = interface.addr;
		taddr.set_port(port);
		cout<<taddr.to_string()<<": Did recv: "<<simulator.current_tick()<<endl;

		if(simulator.current_tick() >= 10) return;

		interface.send(simulator, taddr, addr, std::move(message));
	}

	void did_close() override {}
};

int main() {
	Simulator simulator;
	NetworkConditioner nc;
	Network<NetworkConditioner> network(nc);
	auto& i1 = network.get_or_create_interface(SocketAddress::from_string("192.168.0.1:0"));
	auto& i2 = network.get_or_create_interface(SocketAddress::from_string("192.168.0.2:0"));

	PingPongNode<NetworkInterface<Network<NetworkConditioner>>> p1(simulator), p2(simulator);
	i1.bind(p1, 8000);
	i2.bind(p2, 9000);

	i1.send(
		simulator,
		SocketAddress::from_string("192.168.0.1:8000"),
		SocketAddress::from_string("192.168.0.2:9000"),
		Buffer({0,1,2,3,4}, 5)
	);

	cout<<"Simulation start"<<endl;

	simulator.run();

	cout<<"Simulation end"<<endl;

	return 0;
}
