#include "marlin/pubsub/ABCInterface.hpp"

#include <marlin/net/core/EventLoop.hpp>

int main() {
	marlin::pubsub::ABCInterface abcIface;
	return marlin::net::EventLoop::run();
}
