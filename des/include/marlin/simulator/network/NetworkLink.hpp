#ifndef MARLIN_SIMULATOR_NETWORK_NETWORKLINK_HPP
#define MARLIN_SIMULATOR_NETWORK_NETWORKLINK_HPP

#include "marlin/simulator/network/DataOnLinkEvent.hpp"


namespace marlin {
namespace simulator {

template<
	typename EM,
	typename MT,
	typename SrcType,
	typename DstType
>
class NetworkLink {
public:
	using EventManager = EM;
	using MessageType = MT;

	SrcType &src;
	DstType &dst;

	using SelfType = NetworkLink<
		EventManager,
		MessageType,
		SrcType,
		DstType
	>;

	NetworkLink(
		SrcType &src,
		DstType &dst
	) : src(src), dst(dst) {}

	using SrcEventType = DataOnLinkEvent<EventManager, MessageType, SrcType, SelfType&>;

	void send_to_src(
		EventManager& manager,
		uint64_t current_tick,
		MessageType&& message
	) {
		auto out_tick = get_out_tick(current_tick);
		if(out_tick == 0) return;

		manager.add_event(std::make_shared<SrcEventType>(
			out_tick,
			std::move(message),
			src,
			*this
		));
	}

	using DstEventType = DataOnLinkEvent<EventManager, MessageType, DstType, SelfType&>;

	void send_to_dst(
		EventManager& manager,
		uint64_t current_tick,
		MessageType&& message
	) {
		auto out_tick = get_out_tick(current_tick);
		if(out_tick == 0) return;

		manager.add_event(std::make_shared<DstEventType>(
			out_tick,
			std::move(message),
			dst,
			*this
		));
	}

	virtual uint64_t get_out_tick(uint64_t in_tick) {
		return in_tick;
	}
};

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_NETWORK_NETWORKLINK_HPP
