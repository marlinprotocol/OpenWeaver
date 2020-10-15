#ifndef MARLIN_CORE_TRANSPORTSCAFFOLD_HPP
#define MARLIN_CORE_TRANSPORTSCAFFOLD_HPP

#include <marlin/core/messages/BaseMessage.hpp>
#include <marlin/core/SocketAddress.hpp>
#include <marlin/core/TransportManager.hpp>
#include <marlin/core/Buffer.hpp>

#include <type_traits>

namespace marlin {
namespace core {

template<typename T>
struct MessageTypeSelector {
	using MessageType = core::BaseMessage;
	static constexpr bool value = false;
};

template<
	typename TransportType,
	typename DelegateType,
	typename BaseTransportType
>
class TransportScaffold {
	using BaseMessageType = typename MessageTypeSelector<BaseTransportType>::MessageType;
public:
	using MessageType = std::conditional_t<
		MessageTypeSelector<TransportType>::value,
		typename MessageTypeSelector<TransportType>::MessageType,
		BaseMessageType
	>;

	core::SocketAddress src_addr;
	core::SocketAddress dst_addr;
protected:
	BaseTransportType base_transport;
	core::TransportManager<TransportType>& transport_manager;
public:
	TransportScaffold(
		core::SocketAddress src_addr,
		core::SocketAddress dst_addr,
		BaseTransportType base_transport,
		core::TransportManager<TransportType>& transport_manager
	);
	TransportScaffold(TransportScaffold const&) = delete;
	TransportScaffold& operator=(TransportScaffold const&) = delete;

	DelegateType* delegate = nullptr;

	void setup(DelegateType* delegate);

	// BaseTransportType delegate
	template<typename... Args>
	void did_dial(BaseTransportType base_transport, Args&&... args);
	void did_recv(BaseTransportType base_transport, BaseMessageType&& bytes);
	void did_send(BaseTransportType base_transport, BaseMessageType&& bytes);
	void did_close(BaseTransportType base_transport, uint16_t reason = 0);

	// Actions
	int send(MessageType&& buf);
	void close(uint16_t reason = 0);

	bool is_internal();
};


template<
	typename DelegateType,
	template<typename> typename BaseTransportTemplate,
	template<typename, template<typename> typename, typename...> typename TransportTemplate,
	typename... TArgs
>
using SugaredTransportScaffold = TransportScaffold<
	TransportTemplate<DelegateType, BaseTransportTemplate, TArgs...>,
	DelegateType,
	BaseTransportTemplate<TransportTemplate<DelegateType, BaseTransportTemplate, TArgs...>>&
>;

}  // namespace core
}  // namespace marlin

#include "TransportScaffold.ipp"

#endif  // MARLIN_CORE_TRANSPORTSCAFFOLD_HPP
