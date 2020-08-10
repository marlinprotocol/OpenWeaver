#include <iostream>
#include "marlin/simulator/core/Simulator.hpp"
#include "marlin/simulator/timer/Timer.hpp"

using namespace marlin::simulator;
using namespace std;


class TimerDelegate {
public:
	Timer* timer;
	void timer_cb() {
		cout<<"Timer tick: "<<Simulator::default_instance.current_tick()<<endl;

		if(Simulator::default_instance.current_tick() >= 10) {
			timer->stop();
			return;
		}
	}
};

int main() {
	auto& simulator = Simulator::default_instance;
	TimerDelegate d;
	Timer t(&d);
	d.timer = &t;

	t.start<TimerDelegate, &TimerDelegate::timer_cb>(1, 2);

	cout<<"Simulation start"<<endl;

	simulator.run();

	cout<<"Simulation end"<<endl;

	return 0;
}
