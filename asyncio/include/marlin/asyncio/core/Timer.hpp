/*! \file Timer.hpp
*/

#ifndef MARLIN_CORE_TIMER_HPP
#define MARLIN_CORE_TIMER_HPP

#include <uv.h>
#include <type_traits>
#include <marlin/simulator/timer/Timer.hpp>


namespace marlin {
namespace asyncio {

#ifdef MARLIN_ASYNCIO_SIMULATOR

using Timer = simulator::Timer;

#else

class Timer {
private:
	using Self = Timer;

	uv_timer_t* timer;
	void* data = nullptr;

	static void timer_close_cb(uv_handle_t* handle) {
		delete (uv_timer_t*)handle;
	}

	template<typename DelegateType, void (DelegateType::*callback)()>
	static void timer_cb(uv_timer_t* handle) {
		auto& timer = *(Self*)handle->data;
		(((DelegateType*)(timer.delegate))->*callback)();
	}

	template<typename DelegateType, typename DataType, void (DelegateType::*callback)(DataType&)>
	static void timer_cb(uv_timer_t* handle) {
		auto& timer = *(Self*)handle->data;
		(((DelegateType*)(timer.delegate))->*callback)(*(DataType*)timer.data);
	}
public:
	void* delegate;

	template<typename DelegateType>
	Timer(DelegateType* delegate) : delegate(delegate) {
		timer = new uv_timer_t();
		timer->data = this;
		uv_timer_init(uv_default_loop(), timer);
	}

	template<typename DataType>
	void set_data(DataType* data) {
		this->data = (void*)data;
	}

	template<typename DelegateType, void (DelegateType::*callback)()>
	void start(uint64_t timeout, uint64_t repeat) {
		uv_timer_start(timer, timer_cb<DelegateType, callback>, timeout, repeat);
	}

	template<typename DelegateType, typename DataType, void (DelegateType::*callback)(DataType&)>
	void start(uint64_t timeout, uint64_t repeat) {
		uv_timer_start(timer, timer_cb<DelegateType, DataType, callback>, timeout, repeat);
	}

	void stop() {
		uv_timer_stop(timer);
	}

	~Timer() {
		uv_timer_stop(timer);
		uv_close((uv_handle_t*)timer, timer_close_cb);
	}
};

#endif

} // namespace asyncio
} // namespace marlin

#endif // MARLIN_CORE_TIMER_HPP
