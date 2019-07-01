#ifndef MARLIN_NET_TRANSPORTMANAGER_HPP
#define MARLIN_NET_TRANSPORTMANAGER_HPP

#include <unordered_map>
#include <utility>
#include "SocketAddress.hpp"

namespace marlin {
namespace net {

template<typename TransportType>
class TransportManager {
	std::unordered_map<
		SocketAddress,
		TransportType
	> transport_map;
public:
	TransportType *get(SocketAddress const &addr) {
		auto iter = transport_map.find(addr);
		if(iter == transport_map.end()) {
			return nullptr;
		}

		return &iter->second;
	}

	template<class... Args>
	std::pair<TransportType *, bool> get_or_create(
		SocketAddress const &addr,
		Args&&... args
	) {
		auto res = transport_map.try_emplace(
			addr,
			std::forward<Args>(args)...
		);
		return std::make_pair(&res.first->second, res.second);
	}
};

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_TRANSPORTMANAGER_HPP
