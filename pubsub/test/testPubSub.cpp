#include "gtest/gtest.h"
#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/pubsub/witness/BloomWitnesser.hpp>
#include <marlin/pubsub/attestation/EmptyAttester.hpp>
#include <marlin/mtest/NetworkFixture.hpp>
#include "marlin/simulator/transport/SimulatedTransportFactory.hpp"
#include "marlin/simulator/network/Network.hpp"

using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::stream;
using namespace marlin::pubsub;
using namespace marlin::simulator;
using namespace std;

using namespace marlin::stream;
class PubSubNodeDelegate;
using PubSubNodeType = PubSubNode<PubSubNodeDelegate, true, true, true, EmptyAttester, BloomWitnesser>;

class PubSubNodeDelegate {
public:
    std::vector<uint16_t> channels = {1, 2};

    void did_unsubscribe(PubSubNodeType &, uint16_t channel) {
        SPDLOG_INFO("Did unsubscribe: {}", channel);
    }

    void did_subscribe(PubSubNodeType &ps, uint16_t channel) {
        uint8_t ssk[crypto_box_SECRETKEYBYTES];
        uint8_t spk[crypto_box_PUBLICKEYBYTES];

        crypto_box_keypair(spk, ssk);

        uint8_t message[100000];
        int map_size = ps.conn_map.size();
        ps.send_message_on_channel(channel, message, 100000);
        SPDLOG_INFO("Did subscribe: {}, {}", channel, map_size);
        if (map_size > 0){
            for(auto& [addr, conns] : ps.conn_map){
                SPDLOG_INFO("addr {}", addr.to_string());
                SPDLOG_INFO("size {}",conns.sol_conns.size());
                for (
                        auto it = conns.sol_conns.begin();
                        it != conns.sol_conns.end();
                        it++
                        ){
                    bool is_active = (*it)->is_active();
                    SPDLOG_INFO(" dst addr {}, {}, {}", (*it)->dst_addr.to_string(), (*it)->src_addr.to_string(), is_active);
                    ps.send_UNSUBSCRIBE(**it,channel);
                }
            }
        }

    }

    void did_recv(
            PubSubNodeType &,
            Buffer &&,
            typename PubSubNodeType::MessageHeaderType header,
            uint16_t channel,
            uint64_t message_id
    ) {
        SPDLOG_INFO(
                "Received message {:spn} on channel {} with witness {}",
                spdlog::to_hex((uint8_t*)&message_id, ((uint8_t*)&message_id) + 8),
                channel,
                spdlog::to_hex(header.witness_data, header.witness_data+header.witness_size)
        );
    }

    void manage_subscriptions(
            SocketAddress,
            size_t,
            typename PubSubNodeType::TransportSet& ,
            typename PubSubNodeType::TransportSet&
    ){
    }

    /*
    void manage_subscriptions(
            SocketAddress baddr,
            size_t max_sol_conns,
            typename PubSubNodeType::TransportSet& sol_conns,
            typename PubSubNodeType::TransportSet& sol_standby_conns
    ) {
        SPDLOG_DEBUG(
                "{} port sol_conns size: {}",__FUNCTION__ ,
                sol_conns.size()
        );

        if (sol_conns.size() >= max_sol_conns) {
            auto* toReplaceTransport = sol_conns.find_max_rtt_transport();
            if (toReplaceTransport != nullptr) {
                SPDLOG_DEBUG("Moving address: {} from sol_conns to sol_standby_conns",
                             toReplaceTransport->dst_addr.to_string()
                );

                std::for_each(
                        channels.begin(),
                        channels.end(),
                        [&] (uint16_t channel) {
                            ps->send_UNSUBSCRIBE(*toReplaceTransport, channel);
                        }
                );

                ps->remove_conn(sol_conns, *toReplaceTransport);
                ps->add_sol_standby_conn(baddr, *toReplaceTransport);
            }
        }

        if (sol_conns.size() < max_sol_conns) {
            auto* toReplaceWithTransport = sol_standby_conns.find_min_rtt_transport();
            if ( toReplaceWithTransport != nullptr) {
                auto* toReplaceWithTransport = sol_standby_conns.find_min_rtt_transport();
                SPDLOG_DEBUG("Moving address: {} from sol_standby_conns to sol_conns",
                             toReplaceWithTransport->dst_addr.to_string()
                );
                ps->add_sol_conn(baddr, *toReplaceWithTransport);
            }
        }

        for (auto* transport : sol_standby_conns) {
            auto* remote_static_pk = transport->get_remote_static_pk();
            SPDLOG_INFO(
                    "Node {:spn}: Standby conn: {:spn}: rtt: {}",
                    spdlog::to_hex(static_pk, static_pk + crypto_box_PUBLICKEYBYTES),
                    spdlog::to_hex(remote_static_pk, remote_static_pk + crypto_box_PUBLICKEYBYTES),
                    transport->get_rtt()
            );
        }

        for (auto* transport : sol_conns) {
            auto* remote_static_pk = transport->get_remote_static_pk();
            SPDLOG_INFO(
                    "Node {:spn}: Sol conn: {:spn}: rtt: {}",
                    spdlog::to_hex(static_pk, static_pk + crypto_box_PUBLICKEYBYTES),
                    spdlog::to_hex(remote_static_pk, remote_static_pk + crypto_box_PUBLICKEYBYTES),
                    transport->get_rtt()
            );
        }
    }
    */
};

TEST(PubSubConnectionTest, First) {
    bool delegate_called = true;
//    auto& simulator = Simulator::default_instance;
    uint8_t static_sk[crypto_box_SECRETKEYBYTES];
    uint8_t static_pk[crypto_box_PUBLICKEYBYTES];

    spdlog::set_level(spdlog::level::debug);

    crypto_box_keypair(static_pk, static_sk);

    PubSubNodeDelegate b_del;

    size_t max_sol_conn = 10;
    size_t max_unsol_conn = 1;

    auto addr = SocketAddress::from_string("127.0.0.1:8000");

    auto b = new PubSubNode<PubSubNodeDelegate, true, true, true, EmptyAttester, BloomWitnesser>(addr, max_sol_conn, max_unsol_conn, static_sk, {}, std::tie(static_pk));
    b->delegate = &b_del;

    auto addr2 = SocketAddress::from_string("127.0.0.1:8001");
    auto b2 = new PubSubNode<PubSubNodeDelegate, true, true, true, EmptyAttester, BloomWitnesser>(addr2, max_sol_conn, max_unsol_conn, static_sk, {}, std::tie(static_pk));
    b2->delegate = &b_del;

    SPDLOG_INFO("Start");

    b->dial(addr2, static_pk);
    EXPECT_TRUE(delegate_called);
    EventLoop::run();
//    simulator.run();
}