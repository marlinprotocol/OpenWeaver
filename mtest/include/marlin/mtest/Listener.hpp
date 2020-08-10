#ifndef MARLIN_MTEST_LISTENER_HPP
#define MARLIN_MTEST_LISTENER_HPP

#include <marlin/simulator/core/Simulator.hpp>
#include <marlin/simulator/transport/SimulatedTransportFactory.hpp>
#include <marlin/simulator/network/Network.hpp>


namespace marlin {
namespace mtest {

template<typename NetworkInterfaceType>
struct Listener final : public simulator::NetworkListener<NetworkInterfaceType> {
	std::function<void(
		NetworkInterfaceType&,
		uint16_t,
		core::SocketAddress const&,
		core::Buffer&&
	)> t_did_recv = [](auto&&...) {};
	std::function<void()> t_did_close = []() {};

	void did_recv(
		NetworkInterfaceType& interface,
		uint16_t port,
		core::SocketAddress const& addr,
		core::Buffer&& packet
	) override {
		t_did_recv(interface, port, addr, std::move(packet));
	}

	void did_close() override {
		t_did_close();
	}
};

} // namespace mtest
} // namespace marlin

#endif // MARLIN_MTEST_LISTENER_HPP
