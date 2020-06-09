#ifndef MARLIN_MTEST_NETWORKFIXTURE_HPP
#define MARLIN_MTEST_NETWORKFIXTURE_HPP

#include <marlin/simulator/core/Simulator.hpp>
#include <marlin/simulator/transport/SimulatedTransportFactory.hpp>
#include <marlin/simulator/network/Network.hpp>

#include "Listener.hpp"

namespace marlin {
namespace mtest {

template<typename NetworkConditionerType = simulator::NetworkConditioner>
struct NetworkFixture : public ::testing::Test {
	simulator::Simulator& simulator = simulator::Simulator::default_instance;

	NetworkConditionerType nc;

	using NetworkType = simulator::Network<NetworkConditionerType>;
	NetworkType network;

	using NetworkInterfaceType = simulator::NetworkInterface<NetworkType>;
	NetworkInterfaceType& i1;
	NetworkInterfaceType& i2;
	NetworkInterfaceType& i3;
	NetworkInterfaceType& i4;
	NetworkInterfaceType& i5;

	NetworkFixture() :
		network(nc),
		i1(network.get_or_create_interface(core::SocketAddress::from_string("192.168.0.1:0"))),
		i2(network.get_or_create_interface(core::SocketAddress::from_string("192.168.0.2:0"))),
		i3(network.get_or_create_interface(core::SocketAddress::from_string("192.168.0.3:0"))),
		i4(network.get_or_create_interface(core::SocketAddress::from_string("192.168.0.4:0"))),
		i5(network.get_or_create_interface(core::SocketAddress::from_string("192.168.0.5:0"))) {}

	template<typename Delegate>
	using TransportType = simulator::SimulatedTransport<
		simulator::Simulator,
		NetworkInterfaceType,
		Delegate
	>;
	template<typename ListenDelegate, typename TransportDelegate>
	using TransportFactoryType = simulator::SimulatedTransportFactory<
		simulator::Simulator,
		NetworkInterfaceType,
		ListenDelegate,
		TransportDelegate
	>;

	using ListenerType = Listener<NetworkInterfaceType>;
};

using DefaultNetworkFixture = NetworkFixture<>;

} // namespace mtest
} // namespace marlin

#endif // MARLIN_MTEST_NETWORKFIXTURE_HPP
