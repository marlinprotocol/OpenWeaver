#include "marlin/bsc/Abci.hpp"

#include <marlin/asyncio/core/EventLoop.hpp>

#include <structopt/app.hpp>

using namespace marlin::bsc;
using namespace marlin::asyncio;
using namespace marlin::core;

struct Delegate {
	template<typename AbciType>
	void did_connect(AbciType& abci) {
		abci.get_block_number();
	}

	template<typename AbciType>
	void did_recv(AbciType&, Buffer&& bytes) {
		SPDLOG_INFO("{:.{}s}", bytes.data(), bytes.size());
	}
};

struct Options {
	std::string datadir;
};
STRUCTOPT(Options, datadir);

int main(int argc, char** argv) {
	try {
		auto options = structopt::app("abci").parse<Options>(argc, argv);

		Abci<Delegate> abci(options.datadir);

		return EventLoop::run();
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}
