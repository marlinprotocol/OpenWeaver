#include "gtest/gtest.h"
#include <marlin/stream/ConnSMFiber.hpp>


using namespace marlin::stream;

struct Terminal {
    Terminal(auto&&) {};
};

TEST(ConnSMFiberTest, Basic) {
	ConnSMFiber<Terminal> testObj(std::make_tuple(std::make_tuple()));

    EXPECT_EQ(testObj.testfn(), 2);
}

TEST(ConnSMFiberTest, Did_Recv) {
	ConnSMFiber<Terminal> testObj(std::make_tuple(std::make_tuple()));

    EXPECT_EQ(testObj.did_recv(nullptr, marlin::core::Buffer(10)), 9);
}