/*! \file EventLoop.hpp
*/

#ifndef MARLIN_NET_EVENTLOOP_HPP
#define MARLIN_NET_EVENTLOOP_HPP

#include <uv.h>


namespace marlin {
namespace net {

class EventLoop {
public:
	static int run() {
		return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	}

	static uint64_t now() {
		return uv_now(uv_default_loop());
	}
};

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_EVENTLOOP_HPP
