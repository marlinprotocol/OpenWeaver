#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <cstring>
#include <algorithm>
#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/pubsub/witness/LpfBloomWitnesser.hpp>
#include <marlin/pubsub/attestation/EmptyAttester.hpp>
#include <sodium.h>

using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::stream;
using namespace marlin::pubsub;
using namespace std;


int main() {
	StakeRequester req("/subgraphs/name/marlinprotocol/staking-arb1", "0xaaaebeba3810b1e6b70781f14b2d72c1cb89c0b2b320c43bb67ff79f562f5ff4");

	return EventLoop::run();
}

