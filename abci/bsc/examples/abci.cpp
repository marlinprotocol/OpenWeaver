#include "marlin/bsc/Abci.hpp"

#include <marlin/asyncio/core/EventLoop.hpp>

#include <structopt/app.hpp>

using namespace marlin::bsc;
using namespace marlin::asyncio;

struct Delegate {
	template<typename AbciType>
	void did_connect(AbciType&) {}
};

struct Options {
	std::string datadir;
};
STRUCTOPT(Options, datadir);

int main(int argc, char** argv) {
	try {
		auto options = structopt::app("abci").parse<Options>(argc, argv);

		Abci<Delegate> abci(options.datadir);
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}
	return EventLoop::run();
}
