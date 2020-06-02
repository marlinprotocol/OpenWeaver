/*! \file EventLoop.hpp
*/

#ifndef MARLIN_CORE_EVENTLOOP_HPP
#define MARLIN_CORE_EVENTLOOP_HPP

#include <uv.h>
#include <marlin/simulator/core/Simulator.hpp>


namespace marlin {
namespace asyncio {

#ifdef MARLIN_ASYNCIO_SIMULATOR

struct EventLoop {
	static int run() {
		simulator::Simulator::default_instance.run();
		return 0;
	}

	static uint64_t now() {
		return simulator::Simulator::default_instance.current_tick();
	}
};

#else

class EventLoop {
public:
	static int run() {
		return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	}

	static uint64_t now() {
		return uv_now(uv_default_loop());
	}
};

#endif

} // namespace asyncio
} // namespace marlin

#endif // MARLIN_CORE_EVENTLOOP_HPP
