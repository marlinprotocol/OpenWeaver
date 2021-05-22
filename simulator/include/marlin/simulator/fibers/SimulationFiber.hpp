#ifndef MARLIN_SIMULATION_FIBERS_SIMULATIONFIBER_HPP
#define MARLIN_SIMULATION_FIBERS_SIMULATIONFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/SocketAddress.hpp>
#include <marlin/simulator/network/NetworkInterface.hpp>

namespace marlin {
namespace simulator {


template<
	typename ExtFabric,
	typename NetworkInterfaceType,
	typename EventManager
>
class SimulationFiber : public NetworkListener<NetworkInterfaceType> {
public:
	using SelfType = SimulationFiber<ExtFabric, NetworkInterfaceType, EventManager>;

	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = true;

	using OuterMessageType = core::Buffer;
	using InnerMessageType = core::Buffer;

private:
	[[no_unique_address]] ExtFabric ext_fabric;

	NetworkInterfaceType& interface;
	EventManager& manager;

	bool is_listening = false;

public:
	core::SocketAddress addr;

	template<typename ExtTupleType>
	SimulationFiber(std::tuple<ExtTupleType, NetworkInterfaceType&, EventManager&>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		interface(std::get<1>(init_tuple)),
		manager(std::get<2>(init_tuple)) {
	}

	int bind(core::SocketAddress const& addr) {
		this->addr = addr;

		return 0;
	}

	int listen() {
		is_listening = true;
		return interface.bind(*this, addr.get_port());
	}

	int dial(core::SocketAddress addr, auto&&... args) {
		if(!is_listening) {
			auto status = listen();
			if(status < 0) {
				return status;
			}
		}

		ext_fabric.did_dial(*this, addr, std::forward<decltype(args)>(args)...);

		return 0;
	}

	void did_recv(
		NetworkInterfaceType&,
		uint16_t,
		core::SocketAddress const& addr,
		core::Buffer&& message
	) override {
		did_recv(std::move(message), addr);
	}

	int did_recv(InnerMessageType&& buf, core::SocketAddress addr) {
		if(buf.size() == 0) {
			return 0;
		}
		return ext_fabric.did_recv(*this, std::move(buf), addr);
	}

	int did_dial(core::SocketAddress addr, auto&&... args) {
		return ext_fabric.did_dial(*this, addr, std::forward<decltype(args)>(args)...);
	}

	int send(InnerMessageType&& buf, core::SocketAddress addr) {
		return interface.send(
			manager,
			this->addr,
			addr,
			std::move(buf)
		);
	}

	int did_send(InnerMessageType&& buf) {
		return ext_fabric.did_send(*this, std::move(buf));
	}

	void did_close() override {}
};

}  // namespace simulator
}  // namespace marlin

#endif  // MARLIN_SIMULATION_FIBERS_SIMULATIONFIBER_HPP
