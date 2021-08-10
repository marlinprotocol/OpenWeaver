#include "gtest/gtest.h"
#include <marlin/stream/ConnSMFiber.hpp>
#include <spdlog/spdlog.h>

using namespace marlin::stream;

struct Terminal {
    Terminal() = default;
    Terminal(auto&&) {}
    auto i(auto&&) {return this;}
    int did_recv(auto&&, marlin::core::Buffer&&) {
        SPDLOG_INFO("Reached did_recv");
        return 0;
    }
};

TEST(ConnSMFiberTest, DataBufferDiscardedOnZeroVersion) {
    Terminal SwitchTerminal;
    ConnSMFiber<Terminal> testObj(std::make_tuple(&SwitchTerminal, 1));
    EXPECT_EQ(testObj.did_recv(nullptr, marlin::core::Buffer({0,0,0}, 3)), -1);
}

TEST(ConnSMFiberTest, DataBufferDiscardedOnConnStateListen) {
    Terminal SwitchTerminal;
    ConnSMFiber<Terminal> testObj(std::make_tuple(&SwitchTerminal, 1));
    EXPECT_EQ(testObj.get_connection_state(), ConnectionState::Listen);
    EXPECT_EQ(testObj.did_recv(nullptr, marlin::core::Buffer({1,0},2)), -1);
    EXPECT_EQ(testObj.get_connection_state(), ConnectionState::Listen);
}

TEST(ConnSMFiberTest, DidDialSetsConnStateToDialSent) {
    Terminal SwitchTerminal;
    ConnSMFiber<Terminal> testObj(std::make_tuple(&SwitchTerminal, 1));
    EXPECT_EQ(testObj.get_connection_state(), ConnectionState::Listen);
    testObj.did_dial(&testObj);
    EXPECT_EQ(testObj.get_connection_state(), ConnectionState::DialSent);
}

TEST(ConnSMFiberTest, ConnectionEstablishmentWorks) {
    Terminal SwitchTerminal;
    ConnSMFiber<Terminal> testObj(std::make_tuple(&SwitchTerminal, 1));
    EXPECT_EQ(testObj.get_connection_state(), ConnectionState::Listen);
    testObj.did_dial(&testObj);
    EXPECT_EQ(testObj.get_connection_state(), ConnectionState::DialSent);
    EXPECT_EQ(testObj.did_recv(nullptr, marlin::core::Buffer({1,4},2)), 0);
    EXPECT_EQ(testObj.get_connection_state(), ConnectionState::Established);
}

// TEST(ConnSMFiberTest, DataBufferAcceptedOnCorrectSMState) {
//     Terminal SwitchTerminal;
//     ConnSMFiber<Terminal> testObj(std::make_tuple(&SwitchTerminal, 1));
//     EXPECT_EQ(testObj.did_recv(nullptr, marlin::core::Buffer({1,0},2)), -1);
// }