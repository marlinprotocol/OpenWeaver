#ifndef MARLIN_MTEST_NETWORKFIXTURE_HPP
#define MARLIN_MTEST_NETWORKFIXTURE_HPP

#include <marlin/simulator/core/Simulator.hpp>
#include <marlin/simulator/transport/SimulatedTransportFactory.hpp>
#include <marlin/simulator/network/Network.hpp>


namespace marlin {
namespace mtest {

template<typename NetworkConditionerType = simulator::NetworkConditioner>
struct NetworkFixture : public ::testing::Test {
	simulator::Simulator& simulator = simulator::Simulator::default_instance;

	NetworkConditionerType nc;

	using NetworkType = simulator::Network<NetworkConditionerType>;
	NetworkType network;

	using NetworkInterfaceType = simulator::NetworkInterface<NetworkType>;

	NetworkFixture() : network(nc) {}

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
};

} // namespace mtest
} // namespace marlin

#endif // MARLIN_MTEST_NETWORKFIXTURE_HPP
