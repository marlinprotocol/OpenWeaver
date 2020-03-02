#include <iostream>
#include <vector>
#include "marlin/simulator/core/Simulator.hpp"
#include "marlin/simulator/network/NetworkLink.hpp"

using namespace marlin::simulator;
using namespace std;

struct PingPongNode {
	template<typename NetworkLink>
	void did_recv(
		typename NetworkLink::SrcEventType& event,
		typename NetworkLink::EventManager& manager,
		typename NetworkLink::MessageType&& message,
		NetworkLink& link
	) {
		if(&link.src == this) {
			cout<<"Src did recv: "<<event.get_tick()<<endl;

			if(event.get_tick() >= 10) return;

			link.send_to_dst(manager, event.get_tick(), std::move(message));
		} else {
			cout<<"Dst did recv: "<<event.get_tick()<<endl;

			if(event.get_tick() >= 10) return;

			link.send_to_src(manager, event.get_tick(), std::move(message));
		}
	}
};

template<
	typename EventManager,
	typename MessageType,
	typename SrcType,
	typename DstType
>
struct UniformDelayLink : public NetworkLink<
	EventManager,
	MessageType,
	SrcType,
	DstType
> {
	using BaseLink = NetworkLink<
		EventManager,
		MessageType,
		SrcType,
		DstType
	>;

	using BaseLink::NetworkLink;

	uint64_t get_out_tick(uint64_t in_tick) override {
		return in_tick + 2;
	}

};


int main() {
	Simulator simulator;
	PingPongNode n;
	UniformDelayLink<
		EventQueue,
		std::vector<uint8_t>,
		PingPongNode,
		PingPongNode
	> link(n, n);
	link.send_to_src(simulator.queue, 1, {0,1,2,3,4});

	cout<<"Simulation start"<<endl;

	simulator.run();

	cout<<"Simulation end"<<endl;

	return 0;
}
