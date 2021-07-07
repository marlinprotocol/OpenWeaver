#ifndef MARLIN_UVPP_TCP_HPP
#define MARLIN_UVPP_TCP_HPP

#include <uv.h>

#include <marlin/core/SocketAddress.hpp>


namespace marlin {
namespace uvpp {

struct Empty {};

template<typename ExtraData>
class WriteReq : public uv_write_t {
public:
	[[no_unique_address]] ExtraData extra_data;

	// Construct data in place from arguments
	template<typename... ExtraDataArgs>
	WriteReq(ExtraDataArgs&&... extra_data_args) noexcept(noexcept(ExtraData(std::forward<ExtraDataArgs>(extra_data_args)...))) : extra_data(std::forward<ExtraDataArgs>(extra_data_args)...) {
		data = nullptr;
	}

	//-------- Rule of five begin --------//

	~WriteReq() = default;

	// Not copyable
	WriteReq(WriteReq const& req) = delete;
	WriteReq& operator=(WriteReq const& req) = delete;

	// Not movable
	WriteReq(WriteReq&& req) = delete;
	WriteReq& operator=(WriteReq&& req) = delete;

	//-------- Rule of five end --------//
};

using WriteReqE = WriteReq<Empty>;

}  // namespace uvpp
}  // namespace marlin

#endif  // MARLIN_UVPP_TCP_HPP
