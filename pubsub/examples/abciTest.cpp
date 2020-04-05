#include "marlin/pubsub/ABCInterface.hpp"

#include <marlin/asyncio/core/EventLoop.hpp>

int main() {
	marlin::pubsub::ABCInterface abcIface;
	return marlin::asyncio::EventLoop::run();
}
