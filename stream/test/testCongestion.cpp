#include <marlin/simulator/core/Simulator.hpp>
#include <marlin/simulator/transport/SimulatedTransportFactory.hpp>
#include <marlin/simulator/network/Network.hpp>
#include <marlin/stream/StreamTransportFactory.hpp>
#include <marlin/utils/logs.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <random>
#include "gtest/gtest.h"

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
	size_t recv_count = 0;
	size_t send_count = 0;

	std::function<void(TransportType &transport)> t_did_dial;
	std::function<void(TransportType &transport)> t_did_send;
	std::function<void(TransportType &transport)> t_did_recv;
	std::function<void(TransportType &transport)> t_did_recv_skip_stream;

	int did_recv(
		TransportType &transport [[maybe_unused]],
		Buffer &&packet [[maybe_unused]],
		uint8_t stream_id [[maybe_unused]]
	) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did recv packet: {} bytes on {}",
			transport.src_addr.to_string(),
			transport.dst_addr.to_string(),
			packet.size(),
			stream_id
		);
		t_did_recv(transport);
		return 0;
	}

	void did_send(TransportType &transport, Buffer &&packet [[maybe_unused]]) {
		 SPDLOG_INFO(
		 	"Transport {{ Src: {}, Dst: {} }}: Did send packet: {} bytes number, packet number",
		 	transport.src_addr.to_string(),
		 	transport.dst_addr.to_string(),
		 	packet.size()
		 );
		did_dial(transport);
		t_did_send(transport);
		recv_count++;
	}

	void did_dial(TransportType &transport) {
	    t_did_dial(transport);
	}

	void did_close(TransportType &, uint16_t) {}

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
	) {
		SPDLOG_INFO("Did recv flushstream {} {}", offset, old_offset);
	}

	void did_recv_skip_stream(
		TransportType &transport [[maybe_unused]],
		uint16_t stream_id [[maybe_unused]]
	) {
		SPDLOG_INFO("Did recv skipstream {} {}", transport.src_addr.to_string(), transport.dst_addr.to_string());
		t_did_recv_skip_stream(transport);
		return;
	}

	void did_recv_flush_conf(
		TransportType &transport [[maybe_unused]],
		uint16_t stream_id [[maybe_unused]]
	) {
		Buffer buf = Buffer(100);
		std::memset(buf.data(), 0, 100);
		transport.send(std::move(buf),1);
		SPDLOG_INFO("Did recv flushconf");
	}
};

TEST(StreamTest, CorruptionTest) {
	Simulator& simulator = Simulator::default_instance;
	NetworkConditioner nc;

	nc.drop_pattern = [&](){
        	nc.count++;
        	return false;
	};

	NetworkType network(nc);
   	//spdlog::set_level(spdlog::level::debug);

	network.edit_packet = [&](Buffer& packet [[maybe_unused]]){
	        if (nc.count == 7) {
			SPDLOG_DEBUG("{}",packet.read_uint8(0).value());
        	        std::optional<uint32_t> src_conn_id = packet.read_uint32_le(2);
                	SPDLOG_INFO("src_conn_id {}",src_conn_id.value());
               		int ret = packet.write_uint32_le(2,0);
                	ret++;
       		 }
	};

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

	d.t_did_recv = [&](TransportType &transport [[maybe_unused]]){
		return;
	};

	d.t_did_send = [&](TransportType &transport [[maybe_unused]]){
		return;
	};

	d.t_did_dial = [&](TransportType &transport){
		d.count++;
		if (d.count>1)
			return;
                auto buf = Buffer(350);
                std::memset(buf.data(), 0, 350);
		uint8_t stream_id = 1;
                MARLIN_LOG_INFO("Did dial {}",stream_id);
                transport.send(std::move(buf), stream_id%3);
	};

	s.bind(SocketAddress::from_string("192.168.0.1:8000"));
	s.listen(d);
	c.bind(SocketAddress::from_string("192.168.0.2:8000"));
	c.dial(SocketAddress::from_string("192.168.0.1:8000"), d, static_pk);

	EXPECT_TRUE(true);

	EventLoop::run();
}
/*
TEST(StreamTest, SecondTest) {
        Simulator& simulator = Simulator::default_instance;
        NetworkConditioner nc;

        nc.drop_pattern = [&]() {
                nc.count+=2;
                if(nc.count>23 && nc.count<35)
                {
			if (nc.count != 26)
                        	return true;
		}
                return false;
        };

        NetworkType network(nc);
        spdlog::set_level(spdlog::level::debug);

        network.edit_packet = [&](Buffer& packet [[maybe_unused]]){
                return;
        };

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

        d.t_did_dial = [&](TransportType &transport){
                if (d.count > 0){
                        return;
                }
                ++d.count;

                auto buf = Buffer(s_SIZE*10+350+d.count);
                std::memset(buf.data(), 0, s_SIZE*10+350+d.count);

                auto buf1 = Buffer(s_SIZE*10+350+d.count);
                std::memset(buf.data(), 0, s_SIZE*10+350+d.count);

                auto buf2 = Buffer(s_SIZE*10+350+d.count);
                std::memset(buf.data(), 0, s_SIZE*10+350+d.count);

                auto stream_id = (uint32_t)std::random_device()();
		stream_id = 0;
                SPDLOG_INFO("Did dial {}",stream_id);
                transport.send(std::move(buf), stream_id%3);
                transport.send(std::move(buf1), (stream_id)%3);
                transport.send(std::move(buf2), (stream_id)%3);

        };

	d.t_did_send = [&](TransportType &transport [[maybe_unused]]){
		return;
	};

	d.t_did_recv = [&](TransportType &transport){
		if (d.recv_count == 2){
			SPDLOG_INFO("t_did_recv");
			transport.skip_stream(0);
		}
	};

	int d_count = 0;
	d.t_did_recv_skip_stream = [&](TransportType &transport[[maybe_unused]]){
		if( d_count>0 )
			return;
		auto buf = Buffer(100);
		std::memset(buf.data(), 0, 100);
		transport.send(std::move(buf),1);
		d_count++;
	};

        s.bind(SocketAddress::from_string("192.168.0.1:8000"));
        s.listen(d);
        c.bind(SocketAddress::from_string("192.168.0.2:8000"));
        c.dial(SocketAddress::from_string("192.168.0.1:8000"), d, static_pk);

	simulator.run();
}

TEST(StreamTest, ThirdTest) {

	spdlog::set_level(spdlog::level::trace);

        Simulator& simulator = Simulator::default_instance;
        NetworkConditioner nc;

        nc.drop_pattern = [&]() {
                nc.count+=2;
                if(nc.count>19 && nc.count<35)
                {
                        if (nc.count != 28 && nc.count != 26 && nc.count != 30 )
                                return true;
                }
                return false;
        };

        NetworkType network(nc);

        network.edit_packet = [&](Buffer& packet [[maybe_unused]]){
                return;
        };

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

        d.t_did_dial = [&](TransportType &transport){
                if (d.count > 0){
                        return;
                }
                ++d.count;

                auto buf = Buffer(s_SIZE*10+350+d.count);
                std::memset(buf.data(), 0, s_SIZE*10+350+d.count);

                auto buf1 = Buffer(s_SIZE*10+350+d.count);
                std::memset(buf.data(), 0, s_SIZE*10+350+d.count);

                auto buf2 = Buffer(s_SIZE*10+350+d.count);
                std::memset(buf.data(), 0, s_SIZE*10+350+d.count);

                auto stream_id = (uint32_t)std::random_device()();
                stream_id = 1;
                SPDLOG_INFO("Did dial {}",stream_id);
                transport.send(std::move(buf), stream_id%3);
                transport.send(std::move(buf1), (stream_id)%3);
                transport.send(std::move(buf2), (stream_id)%3);

        };

        d.t_did_send = [&](TransportType &transport[[maybe_unused]]){
                MARLIN_LOG_DEBUG("{}",d.send_count);
		if (d.send_count == 3) {
			d.send_count += 1;
			SPDLOG_INFO("flushstream");
                	transport.flush_stream(1);
		}
        };

	d.t_did_recv = [&](TransportType &transport [[maybe_unused]]){
		d.send_count += 1;
	};

        s.bind(SocketAddress::from_string("192.168.0.1:8000"));
        s.listen(d);
        c.bind(SocketAddress::from_string("192.168.0.2:8000"));
        c.dial(SocketAddress::from_string("192.168.0.1:8000"), d, static_pk);

        simulator.run();
}
*/
