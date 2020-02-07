#include <marlin/net/core/Timer.hpp>
#include <marlin/net/core/EventLoop.hpp>
#include <spdlog/spdlog.h>

using namespace marlin::net;


struct Delegate {
	void timer_callback();
	using TimerType = Timer<Delegate, &Delegate::timer_callback>;

	TimerType timer;

	Delegate() : timer(this) {}
};

void Delegate::timer_callback() {
	SPDLOG_INFO("Timer fired: {}", EventLoop::now());
	timer.start(1000, 0);
}


int main() {
	Delegate d;

	SPDLOG_INFO("Now: {}", EventLoop::now());

	d.timer.start(1000, 0);

	return EventLoop::run();
}
