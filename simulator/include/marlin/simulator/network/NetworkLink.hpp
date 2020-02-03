#ifndef MARLIN_SIMULATOR_NETWORK_NETWORKLINK_HPP
#define MARLIN_SIMULATOR_NETWORK_NETWORKLINK_HPP

#include "marlin/simulator/network/DataOnLinkEvent.hpp"


namespace marlin {
namespace simulator {

template<typename EventManager, typename MessageType>
class NetworkLink {
private:
	std::function<void(MessageType &&, uint64_t, EventManager&)> f_src_did_recv;
	std::function<void(MessageType &&, uint64_t, EventManager&)> f_dst_did_recv;
public:
	NetworkLink(
		std::function<void(MessageType &&, uint64_t, EventManager&)> f_src_did_recv,
		std::function<void(MessageType &&, uint64_t, EventManager&)> f_dst_did_recv
	) : f_src_did_recv(f_src_did_recv), f_dst_did_recv(f_dst_did_recv) {}

	template<typename EventType = DataOnLinkEvent<EventManager, MessageType>>
	void send(MessageType &&message, bool direction, uint64_t current_tick, EventManager& manager) {
		auto out_tick = get_out_tick(current_tick);
		if(out_tick == 0) return;

		if(direction) {
			manager.add_event(std::make_shared<EventType>(
				out_tick,
				std::move(message),
				f_dst_did_recv
			));
		} else {
			manager.add_event(std::make_shared<EventType>(
				out_tick,
				std::move(message),
				f_src_did_recv
			));
		}
	}

	virtual uint64_t get_out_tick(uint64_t in_tick) {
		return in_tick;
	}
};

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_NETWORK_NETWORKLINK_HPP
