#ifndef MARLIN_SIMULATOR_NETWORK_DATAONINTERFACEEVENT_HPP
#define MARLIN_SIMULATOR_NETWORK_DATAONINTERFACEEVENT_HPP

#include "marlin/simulator/core/Event.hpp"

#include <marlin/core/SocketAddress.hpp>


namespace marlin {
namespace simulator {

template<
	typename EventManager,
	typename MessageType,
	typename TargetType
>
class DataOnInterfaceEvent : public Event<EventManager> {
private:
	uint16_t dst;
	core::SocketAddress src;
	MessageType message;
	TargetType& target;

public:
	DataOnInterfaceEvent(
		uint64_t tick,
		uint16_t dst,
		core::SocketAddress const& src,
		MessageType &&message,
		TargetType &target
	) : Event<EventManager>(tick), dst(dst), src(src), message(std::move(message)), target(target) {}
	virtual ~DataOnInterfaceEvent() {}

	virtual void run(EventManager&) override {
		target.did_recv(dst, src, std::move(message));
	}
};

} // namespace simulator
} // namespace marlin

#endif // MARLIN_SIMULATOR_NETWORK_DATAONINTERFACEEVENT_HPP
