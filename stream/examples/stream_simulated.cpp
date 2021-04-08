#define MARLIN_ASYNCIO_SIMULATOR

#include <marlin/simulator/core/Simulator.hpp>
#include <marlin/simulator/transport/SimulatedTransportFactory.hpp>
#include <marlin/simulator/network/Network.hpp>
#include <marlin/stream/StreamTransportFactory.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::simulator;
using namespace marlin::stream;

struct Delegate;

template<typename Delegate>
using SimTransportType = SimulatedTransport<
	Simulator,
	NetworkInterface<Network<NetworkConditioner>>,
	Delegate
>;
template<typename ListenDelegate, typename TransportDelegate>
using SimTransportFactoryType = SimulatedTransportFactory<
	Simulator,
	NetworkInterface<Network<NetworkConditioner>>,
	ListenDelegate,
	TransportDelegate
>;

using TransportType = StreamTransport<Delegate, SimTransportType>;

using NetworkType = Network<NetworkConditioner>;

using NetworkInterfaceType = NetworkInterface<Network<NetworkConditioner>>;

#define l_SIZE 100000000
#define m_SIZE 100000
#define s_SIZE 100

uint8_t static_sk[crypto_box_SECRETKEYBYTES];
uint8_t static_pk[crypto_box_PUBLICKEYBYTES];

struct Delegate {
	size_t count = 0;

	int did_recv(
		TransportType &transport [[maybe_unused]],
		Buffer &&packet [[maybe_unused]],
		uint8_t stream_id [[maybe_unused]]
	) {
		SPDLOG_DEBUG(
			"Transport {{ Src: {}, Dst: {} }}: Did recv packet: {} bytes",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			packet.size()
		);
		return 0;
	}

	void did_send(TransportType &transport, Buffer &&packet [[maybe_unused]]) {
		// SPDLOG_INFO(
		// 	"Transport {{ Src: {}, Dst: {} }}: Did send packet: {} bytes",
		// 	transport.src_addr.to_string(),
		// 	transport.dst_addr.to_string(),
		// 	packet.size()
		// );
		did_dial(transport);
	}

	void did_dial(TransportType &transport) {
		if(count > 0xfff) {
			return;
		}
		if((count & 0xff) == 0) {
			SPDLOG_INFO("Checkpoint: {}", count);
		}
		++count;

		auto buf = Buffer(m_SIZE);
		std::memset(buf.data(), 0, m_SIZE);

		// SPDLOG_INFO("Did dial");

		transport.send(std::move(buf));
	}

	void did_close(TransportType &, uint16_t) {
		SPDLOG_INFO("Transport Closed");
	}

	bool should_accept(SocketAddress const &) {
		return true;
	}

	void did_create_transport(TransportType &transport) {
		transport.setup(this, static_sk);
	}

	void did_recv_flush_stream(
		TransportType &transport [[maybe_unused]],
		uint16_t stream_id [[maybe_unused]],
		uint64_t offset [[maybe_unused]],
		uint64_t old_offset [[maybe_unused]]
	) {}

	void did_recv_skip_stream(
		TransportType &transport [[maybe_unused]],
		uint16_t stream_id [[maybe_unused]]
	) {}

	void did_recv_flush_conf(
		TransportType &transport [[maybe_unused]],
		uint16_t stream_id [[maybe_unused]]
	) {}
};

int main() {
	
	Simulator& simulator = Simulator::default_instance;
	NetworkConditioner nc;
	NetworkType network(nc);

	auto& i1 = network.get_or_create_interface(SocketAddress::from_string("192.168.0.1:0"));
	auto& i2 = network.get_or_create_interface(SocketAddress::from_string("192.168.0.2:0"));

	crypto_box_keypair(static_pk, static_sk);

	SPDLOG_DEBUG(
		"PK: {:spn}\nSK: {:spn}",
		spdlog::to_hex(static_pk, static_pk+32),
		spdlog::to_hex(static_sk, static_sk+32)
	);

	StreamTransportFactory<
		Delegate,
		Delegate,
		SimTransportFactoryType,
		SimTransportType
	> s(i1, simulator), c(i2, simulator);
	Delegate d;

	s.bind(SocketAddress::from_string("192.168.0.1:8000"));
	s.listen(d);
	c.bind(SocketAddress::from_string("192.168.0.2:8000"));
	c.dial(SocketAddress::from_string("192.168.0.1:8000"), d, static_pk);

	return EventLoop::run();
}
