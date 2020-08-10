/*! \file TransportManager.hpp
*/

#ifndef MARLIN_CORE_TRANSPORTMANAGER_HPP
#define MARLIN_CORE_TRANSPORTMANAGER_HPP

#include <unordered_map>
#include <utility>
#include "marlin/core/SocketAddress.hpp"

namespace marlin {
namespace core {

/// Template Class which acts as a helper to store and retreive transport instances of template type against a query address
template<typename TransportType>
class TransportManager {
	std::unordered_map<
		SocketAddress,
		TransportType
	> transport_map;

	// Prevent copy, causes subtle bugs with objects holding onto different instances because of implicit copy somewhere
	TransportManager(TransportManager const&) = delete;
public:
	/// Default constructor
	TransportManager() = default;

	/// Get transport with a given destination address,
	/// returns nullptr if no transport is found
	TransportType *get(SocketAddress const &addr) {
		auto iter = transport_map.find(addr);
		if(iter == transport_map.end()) {
			return nullptr;
		}

		return &iter->second;
	}

	/// Get transport with a given destination address,
	/// creating one if not found
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

	/// Remove transport with the given destination address
	void erase(SocketAddress const &addr) {
		transport_map.erase(addr);
	}
};

} // namespace core
} // namespace marlin

#endif // MARLIN_CORE_TRANSPORTMANAGER_HPP
