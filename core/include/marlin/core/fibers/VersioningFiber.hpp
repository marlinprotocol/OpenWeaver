#ifndef MARLIN_CORE_FIBERS_VERSIONINGFIBER_HPP
#define MARLIN_CORE_FIBERS_VERSIONINGFIBER_HPP

#include <marlin/core/Buffer.hpp>

namespace marlin {
namespace core {

#include <marlin/core/messages/FieldDef.hpp>

template<
	typename BaseMessageType,
	typename VERSION = std::integral_constant<uint8_t, 0>,
	typename MIN_VERSION = std::integral_constant<uint8_t, 0>,
	typename MAX_VERSION = std::integral_constant<uint8_t, 0>
>
struct VersionedMessage {
	MARLIN_MESSAGES_BASE(VersionedMessage);
	MARLIN_MESSAGES_UINT_FIELD(8,, version, 0);
	MARLIN_MESSAGES_PAYLOAD_FIELD(1);

	/// Construct a versioned message with a given payload size
	VersionedMessage(size_t payload_size = 0) : base(1 + payload_size) {
		this->set_version(VERSION());
	}

	/// Validate the versioned message
	[[nodiscard]] bool validate() const {
		return base.payload_buffer().size() > 0 && this->version() >= MIN_VERSION() && this->version() <= MAX_VERSION();
	}
};

#include <marlin/core/messages/FieldUndef.hpp>


template<typename ExtFabric>
class VersioningFiber {
public:
	using SelfType = VersioningFiber<ExtFabric>;

	static constexpr bool is_outer_open = true;
	static constexpr bool is_inner_open = true;

	using OuterMessageType = Buffer;
	using InnerMessageType = VersionedMessage<Buffer>;

private:
	[[no_unique_address]] ExtFabric ext_fabric;

public:
	template<typename ExtTupleType>
	VersioningFiber(std::tuple<ExtTupleType>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)) {
	}

	int did_recv(auto&&, InnerMessageType&& buf, SocketAddress addr) {
		if(!buf.validate()) {
			// Validation failure
			return -1;
		}
		return ext_fabric.i(*this).did_recv(*this, std::move(buf), addr);
	}

	int did_dial(auto&&, SocketAddress addr, auto&&... args) {
		return ext_fabric.i(*this).did_dial(*this, addr, std::forward<decltype(args)>(args)...);
	}

	int send(auto&&, InnerMessageType&& buf, SocketAddress addr) {
		return ext_fabric.o(*this).send(*this, std::move(buf), addr);
	}

	int did_send(auto&&, InnerMessageType&& buf) {
		return ext_fabric.i(*this).did_send(*this, std::move(buf));
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_VERSIONINGFIBER_HPP
