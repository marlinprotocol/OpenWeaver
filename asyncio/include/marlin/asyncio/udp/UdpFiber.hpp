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
	uvpp:UdpE* udp_handle = nullptr;

public:
	UdpFiber() {
		udp_handle = new uvpp::UdpE();
	}
};

}  // namespace asyncio
}  // namespace marlin

#endif  // MARLIN_ASYNCIO_UDP_UDPFIBER_HPP
