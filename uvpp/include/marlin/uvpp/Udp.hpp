#ifndef MARLIN_UVPP_UDP_HPP
#define MARLIN_UVPP_UDP_HPP

#include <uv.h>

#include <marlin/core/SocketAddress.hpp>


namespace marlin {
namespace uvpp {

struct Empty {};

template<typename ExtraData>
class Udp : public uv_udp_t {
public:
	[[no_unique_address]] ExtraData extra_data;

	// Construct data in place from arguments
	template<typename... ExtraDataArgs>
	Udp(ExtraDataArgs&&... extra_data_args) noexcept(noexcept(ExtraData(std::forward<ExtraDataArgs>(extra_data_args)...))) : extra_data(std::forward<ExtraDataArgs>(extra_data_args)...) {
		data = nullptr;
	}

	//-------- Rule of five begin --------//

	~Udp() = default;

	// Not copyable
	Udp(Udp const& udp) = delete;
	Udp& operator=(Udp const& udp) = delete;

	// Not movable
	Udp(Udp&& udp) = delete;
	Udp& operator=(Udp&& udp) = delete;

	//-------- Rule of five end --------//

	bool is_active() {
		return uv_is_active((uv_handle_t*)this) != 0;
	}
};

using UdpE = Udp<Empty>;

}  // namespace uvpp
}  // namespace marlin

#endif  // MARLIN_UVPP_UDP_HPP
