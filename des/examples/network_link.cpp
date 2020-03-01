#include <iostream>
#include <vector>
#include "marlin/simulator/core/Simulator.hpp"
#include "marlin/simulator/network/NetworkLink.hpp"

using namespace marlin::simulator;
using namespace std;


class PingPongNetworkLink final : public NetworkLink<EventQueue, vector<uint8_t>> {
public:
	PingPongNetworkLink() : NetworkLink<EventQueue, vector<uint8_t>>(
		bind(&PingPongNetworkLink::did_recv, this, placeholders::_1, placeholders::_2, placeholders::_3),
		bind(&PingPongNetworkLink::did_recv, this, placeholders::_1, placeholders::_2, placeholders::_3)
	) {}

	uint64_t get_out_tick(uint64_t in_tick) override {
		return in_tick + 2;
	}

	void did_recv(vector<uint8_t>&& message, uint64_t tick, EventQueue& manager) {
		cout<<"Did recv: "<<tick<<endl;

		if(tick >= 10) return;

		send(std::move(message), true, tick, manager);
	}
};

int main() {
	Simulator simulator;
	PingPongNetworkLink link;
	link.send({0,1,2,3,4}, true, 1, simulator.queue);

	cout<<"Simulation start"<<endl;

	simulator.run();

	cout<<"Simulation end"<<endl;

	return 0;
}
