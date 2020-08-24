#include "marlin/bsc/Abci.hpp"

#include <marlin/asyncio/core/EventLoop.hpp>

using namespace marlin::bsc;

struct Delegate {
	template<typename AbciType>
	void did_connect(AbciType&) {}
};

int main() {
	Abci<Delegate> abci("~/.bsc");
	return marlin::asyncio::EventLoop::run();
}
