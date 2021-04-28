#ifndef MARLIN_ASYNCIO_UDP_UDPFIBER_HPP
#define MARLIN_ASYNCIO_UDP_UDPFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/uvpp/Udp.hpp>

namespace marlin {
namespace asyncio {

template<typename ExtFabric>
class UdpFiber {
public:
	using SelfType = UdpFiber<ExtFabric>;

	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = true;

	// using OuterMessageType = core::Buffer;
	using InnerMessageType = core::Buffer;

private:
	[[no_unique_address]] ExtFabric ext_fabric;
	uvpp:UdpE* udp_handle = nullptr;

public:
	template<typename ExtTupleType>
	UdpFiber(std::tuple<ExtTupleType>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)) {
		udp_handle = new uvpp::UdpE();
	}
};

}  // namespace asyncio
}  // namespace marlin

#endif  // MARLIN_ASYNCIO_UDP_UDPFIBER_HPP
