#include <marlin/net/core/Timer.hpp>
#include <marlin/net/core/EventLoop.hpp>
#include <spdlog/spdlog.h>

using namespace marlin::net;


struct Delegate {
	void timer_callback();
	using TimerType = Timer<Delegate>;

	TimerType timer;

	Delegate() : timer(this) {}
};

void Delegate::timer_callback() {
	SPDLOG_INFO("Timer fired: {}", EventLoop::now());
	timer.template start<&Delegate::timer_callback>(1000, 0);
}


int main() {
	Delegate d;

	SPDLOG_INFO("Now: {}", EventLoop::now());

	d.timer.template start<&Delegate::timer_callback>(1000, 0);

	return EventLoop::run();
}
