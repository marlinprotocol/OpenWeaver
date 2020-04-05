/*! \file EventLoop.hpp
*/

#ifndef MARLIN_CORE_EVENTLOOP_HPP
#define MARLIN_CORE_EVENTLOOP_HPP

#include <uv.h>


namespace marlin {
namespace asyncio {

class EventLoop {
public:
	static int run() {
		return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	}

	static uint64_t now() {
		return uv_now(uv_default_loop());
	}
};

} // namespace asyncio
} // namespace marlin

#endif // MARLIN_CORE_EVENTLOOP_HPP
