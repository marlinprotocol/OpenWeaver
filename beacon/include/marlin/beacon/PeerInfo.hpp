#ifndef MARLIN_BEACON_PEERINFO_HPP
#define MARLIN_BEACON_PEERINFO_HPP

#include <array>

namespace marlin {
namespace beacon {

struct PeerInfo {
	uint64_t last_seen = 0;
	std::array<uint8_t, 32> key = {};
	std::array<uint8_t, 20> address = {};
};

}  // namespace beacon
}  // namespace marlin

#endif  // MARLIN_BEACON_PEERINFO_HPP

