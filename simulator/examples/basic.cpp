#include <iostream>
#include "marlin/simulator/core/Simulator.hpp"

using namespace marlin::simulator;
using namespace std;


class RepeatedEvent final : public Event<Simulator> {
public:
	RepeatedEvent(uint64_t tick) : Event<Simulator>(tick) {}

	void run(Simulator &manager) override {
		cout<<"Event fired: "<<tick<<endl;

		if(tick >= 10) return;

		auto new_event = std::make_shared<RepeatedEvent>(tick+1);
		manager.add_event(new_event);
	}
};

int main() {
	auto& simulator = Simulator::default_instance;
	simulator.add_event(std::make_shared<RepeatedEvent>(1));

	cout<<"Simulation start"<<endl;

	simulator.run();

	cout<<"Simulation end"<<endl;

	return 0;
}
