#ifndef MARLIN_CORE_FIBERS_VERSIONINGFIBER_HPP
#define MARLIN_CORE_FIBERS_VERSIONINGFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>

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
class VersioningFiber : public FiberScaffold<
	VersioningFiber<ExtFabric>,
	ExtFabric,
	VersionedMessage<Buffer>,
	Buffer
> {
public:
	using SelfType = VersioningFiber<ExtFabric>;
	using FiberScaffoldType = FiberScaffold<
		SelfType,
		ExtFabric,
		VersionedMessage<Buffer>,
		Buffer
	>;

	using typename FiberScaffoldType::InnerMessageType;
	using typename FiberScaffoldType::OuterMessageType;

	using FiberScaffoldType::FiberScaffoldType;

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "did_recv"_tag) {
			return did_recv(std::forward<decltype(args)>(args)...);
		} else {
			return FiberScaffoldType::template outer_call<tag>(std::forward<decltype(args)>(args)...);
		}
	}

	template<uint32_t tag>
	auto inner_call(auto&&... args) {
		if constexpr (tag == "send"_tag) {
			return send(std::forward<decltype(args)>(args)...);
		} else {
			return FiberScaffoldType::template inner_call<tag>(std::forward<decltype(args)>(args)...);
		}
	}

private:
	int did_recv(auto&&, InnerMessageType&& buf, SocketAddress addr) {
		if(!buf.validate()) {
			// Validation failure
			return -1;
		}
		return FiberScaffoldType::template outer_call<"did_recv"_tag>(*this, std::move(buf), addr);
	}

	int send(auto&&, InnerMessageType&& buf, SocketAddress addr) {
		return FiberScaffoldType::template inner_call<"send"_tag>(*this, std::move(buf), addr);
	}
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_VERSIONINGFIBER_HPP
