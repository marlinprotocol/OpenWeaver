#ifndef MARLIN_ASYNCIO_UDP_UDPFIBER_HPP
#define MARLIN_ASYNCIO_UDP_UDPFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/uvpp/Udp.hpp>

#include <spdlog/spdlog.h>

namespace marlin {
namespace asyncio {

template<typename ExtFabric>
class UdpFiber {
public:
	using SelfType = UdpFiber<ExtFabric>;

	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = true;

	using OuterMessageType = void;
	using InnerMessageType = core::Buffer;

private:
	[[no_unique_address]] ExtFabric ext_fabric;
	uvpp::UdpE* udp_handle = nullptr;

public:
	template<typename ExtTupleType>
	UdpFiber(std::tuple<ExtTupleType>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)) {
		udp_handle = new uvpp::UdpE();
	}

	[[nodiscard]] int bind(core::SocketAddress const& addr) {
		int res = uv_udp_init(uv_default_loop(), udp_handle);
		if (res < 0) {
			SPDLOG_ERROR(
				"Asyncio: Socket {}: Init error: {}",
				addr.to_string(),
				res
			);
			return res;
		}

		res = uv_udp_bind(
			udp_handle,
			reinterpret_cast<sockaddr const*>(&addr),
			0
		);
		if (res < 0) {
			SPDLOG_ERROR(
				"Asyncio: Socket {}: Bind error: {}",
				addr.to_string(),
				res
			);
			return res;
		}

		return 0;
	}
};

}  // namespace asyncio
}  // namespace marlin

#endif  // MARLIN_ASYNCIO_UDP_UDPFIBER_HPP
