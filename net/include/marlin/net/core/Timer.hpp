/*! \file Timer.hpp
*/

#ifndef MARLIN_NET_TIMER_HPP
#define MARLIN_NET_TIMER_HPP

#include <uv.h>


namespace marlin {
namespace net {


template<
	typename DelegateType
>
class Timer {
private:
	using Self = Timer<DelegateType>;

	uv_timer_t* timer;

	static void timer_close_cb(uv_handle_t* handle) {
		delete (uv_timer_t*)handle;
	}

	template<void (DelegateType::*callback)()>
	static void timer_cb(uv_timer_t* handle) {
		auto& timer = *(Self*)handle->data;
		(timer.delegate->*callback)();
	}
public:
	DelegateType* delegate;

	Timer(DelegateType* delegate) : delegate(delegate) {
		timer = new uv_timer_t();
		timer->data = this;
		uv_timer_init(uv_default_loop(), timer);
	}

	template<void (DelegateType::*callback)()>
	void start(uint64_t timeout, uint64_t repeat) {
		uv_timer_start(timer, timer_cb<callback>, timeout, repeat);
	}

	void stop() {
		uv_timer_stop(timer);
	}

	~Timer() {
		uv_timer_stop(timer);
		uv_close((uv_handle_t*)timer, timer_close_cb);
	}
};

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_TIMER_HPP
