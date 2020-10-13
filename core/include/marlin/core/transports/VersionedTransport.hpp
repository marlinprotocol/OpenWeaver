#ifndef MARLIN_CORE_TRANSPORTS_VERSIONEDTRANSPORT
#define MARLIN_CORE_TRANSPORTS_VERSIONEDTRANSPORT

#include <marlin/core/transports/TransportScaffold.hpp>

namespace marlin {
namespace core {

#include <marlin/core/messages/FieldDef.hpp>

template<
	typename BaseMessageType,
	uint8_t VERSION = 0,
	uint8_t MIN_VERSION = 0,
	uint8_t MAX_VERSION = 0
>
struct VersionedMessage {
	MARLIN_MESSAGES_BASE(VersionedMessage);
	MARLIN_MESSAGES_UINT_FIELD(8,, version, 0);
	MARLIN_MESSAGES_PAYLOAD_FIELD(1);

	/// Construct a versioned message with a given payload size
	VersionedMessage(size_t payload_size = 0) : base(1 + payload_size) {
		this->set_version(VERSION);
	}

	/// Validate the versioned message
	[[nodiscard]] bool validate() const {
		return base.payload_buffer().size() > 0 && this->version() >= MIN_VERSION && this->version() <= MAX_VERSION;
	}
};

template<
	typename DelegateType,
	template<typename> typename BaseTransportTemplate,
	uint8_t VERSION = 0,
	uint8_t MIN_VERSION = 0,
	uint8_t MAX_VERSION = 0
>
class VersionedTransport : public TransportScaffold<
	VersionedTransport<DelegateType, BaseTransportTemplate, VERSION, MIN_VERSION, MAX_VERSION>,
	DelegateType,
	BaseTransportTemplate<VersionedTransport<DelegateType, BaseTransportTemplate, VERSION, MIN_VERSION, MAX_VERSION>>&
> {
public:
	using SelfType = VersionedTransport<DelegateType, BaseTransportTemplate, VERSION, MIN_VERSION, MAX_VERSION>;
	using BaseTransportType = BaseTransportTemplate<SelfType>;
	using TransportScaffoldType = TransportScaffold<SelfType, DelegateType, BaseTransportType&>;

	using TransportScaffoldType::src_addr;
	using TransportScaffoldType::dst_addr;
private:
	using TransportScaffoldType::base_transport;
	using TransportScaffoldType::transport_manager;
public:
	using MessageType = VersionedMessage<typename BaseTransportType::MessageType>;
	static_assert(std::is_same_v<MessageType, typename TransportScaffoldType::MessageType>);

	using TransportScaffoldType::delegate;

	using TransportScaffoldType::TransportScaffoldType;

	void did_recv(BaseTransportType&, MessageType&& buf) {
		if(!buf.validate()) {
			// Validation failure
			return;
		}
		delegate->did_recv(*this, std::move(buf));
	}
};

template<
	typename DelegateType,
	template<typename> typename BaseTransportTemplate,
	uint8_t VERSION,
	uint8_t MIN_VERSION,
	uint8_t MAX_VERSION
>
struct MessageTypeSelector<VersionedTransport<DelegateType, BaseTransportTemplate, VERSION, MIN_VERSION, MAX_VERSION>> {
	using MessageType = VersionedMessage<typename BaseTransportTemplate<VersionedTransport<DelegateType, BaseTransportTemplate, VERSION, MIN_VERSION, MAX_VERSION>>::MessageType>;
	static constexpr bool value = true;
};

#include <marlin/core/messages/FieldUndef.hpp>

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_TRANSPORTS_VERSIONEDTRANSPORT
