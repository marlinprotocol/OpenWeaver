/*! \file Timer.hpp
*/

#ifndef MARLIN_SIMULATOR_TIMER_TIMER_HPP
#define MARLIN_SIMULATOR_TIMER_TIMER_HPP

#include <type_traits>
#include "marlin/simulator/core/Simulator.hpp"
#include "marlin/simulator/timer/TimerEvent.hpp"


namespace marlin {
namespace simulator {


// Simulated timer, hardcoded Simulator instance as event manager
class Timer {
private:
	using Self = Timer;

	void* data = nullptr;

	template<typename DelegateType, void (DelegateType::*callback)()>
	void timer_cb() {
		(((DelegateType*)(delegate))->*callback)();

		if(repeat > 0) {
			start<DelegateType, callback>(repeat, repeat);
		}
	}

	template<typename DelegateType, typename DataType, void (DelegateType::*callback)(DataType&)>
	void timer_cb() {
		(((DelegateType*)(delegate))->*callback)(*(DataType*)data);

		if(repeat > 0) {
			start<DelegateType, DataType, callback>(repeat, repeat);
		}
	}

	uint64_t repeat = 0;
	std::shared_ptr<Event<Simulator>> next_event = nullptr;
public:
	void* delegate;

	template<typename DelegateType>
	Timer(DelegateType* delegate) : delegate(delegate) {}

	template<typename DataType>
	void set_data(DataType* data) {
		this->data = (void*)data;
	}

	template<typename DelegateType, void (DelegateType::*callback)()>
	void start(uint64_t timeout, uint64_t repeat) {
		this->repeat = repeat;
		next_event = std::make_shared<TimerEvent<
			Simulator,
			Self,
			&Self::timer_cb<DelegateType, callback>
		>>(
			Simulator::default_instance.current_tick() + timeout,
			*this
		);

		Simulator::default_instance.add_event(next_event);
	}

	template<typename DelegateType, typename DataType, void (DelegateType::*callback)(DataType&)>
	void start(uint64_t timeout, uint64_t repeat) {
		this->repeat = repeat;
		auto event = std::make_shared<TimerEvent<
			Simulator,
			Self,
			&Self::timer_cb<DelegateType, DataType, callback>
		>>(
			Simulator::default_instance.current_tick() + timeout,
			*this
		);

		Simulator::default_instance.add_event(event);
	}

	void stop() {
		repeat = 0;
		if(next_event != nullptr){
			Simulator::default_instance.remove_event(next_event);
		}
	}

	~Timer() {
		stop();
	}
};

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_TIMER_TIMER_HPP
