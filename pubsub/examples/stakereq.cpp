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


class PubSubNodeDelegate {
private:
	using PubSubNodeType = PubSubNode<PubSubNodeDelegate, true, true, true, EmptyAttester, LpfBloomWitnesser>;

public:
	std::vector<uint16_t> channels = {100, 101};

	void did_unsubscribe(PubSubNodeType &, uint16_t channel) {
		SPDLOG_INFO("Did unsubscribe: {}", channel);
	}

	void did_subscribe(PubSubNodeType &ps, uint16_t channel) {
		uint8_t message[100000];
		ps.send_message_on_channel(channel, message, 100000);
		SPDLOG_INFO("Did subscribe: {}", channel);
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
		std::array<uint8_t, 20>,
		size_t,
		typename PubSubNodeType::TransportSet&,
		typename PubSubNodeType::TransportSet&
	) {

	}
};

int main() {
	StakeRequester<PubSubNodeDelegate> req("/subgraphs/name/marlinprotocol/staking", "0xaaaebeba3810b1e6b70781f14b2d72c1cb89c0b2b320c43bb67ff79f562f5ff4");

	return EventLoop::run();
}

