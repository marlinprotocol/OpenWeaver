#ifndef MARLIN_CORE_TRANSPORTSCAFFOLD_HPP
#define MARLIN_CORE_TRANSPORTSCAFFOLD_HPP

#include <marlin/core/messages/BaseMessage.hpp>
#include <marlin/core/SocketAddress.hpp>
#include <marlin/core/TransportManager.hpp>
#include <marlin/core/Buffer.hpp>

#include <type_traits>

namespace marlin {
namespace core {

template<typename T, typename = void>
struct MessageTypeSelector {
	using MessageType = core::BaseMessage;
};

template<typename T>
struct MessageTypeSelector<T, std::enable_if_t<std::is_object_v<typename std::decay_t<T>::MessageType>>> {
	using MessageType = typename std::decay_t<T>::MessageType;
};

template<
	typename TransportType,
	typename DelegateType,
	typename BaseTransportType
>
class TransportScaffold {
public:
	using MessageType = typename MessageTypeSelector<BaseTransportType>::MessageType;

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
	void did_dial(BaseTransportType base_transport);
	void did_recv(BaseTransportType base_transport, core::Buffer&& bytes);
	void did_send(BaseTransportType base_transport, core::Buffer&& bytes);
	void did_close(BaseTransportType base_transport, uint16_t reason = 0);

	// Actions
	int send(MessageType&& buf);
	void close(uint16_t reason = 0);

	bool is_internal();
};

}  // namespace core
}  // namespace marlin

#include "TransportScaffold.ipp"

#endif  // MARLIN_CORE_TRANSPORTSCAFFOLD_HPP
