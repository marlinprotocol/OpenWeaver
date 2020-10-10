namespace marlin {
namespace core {

//---------------- Helper macros begin ----------------//

#define TRANSPORTSCAFFOLD_TEMPLATE \
	typename TransportType, \
	typename DelegateType, \
	typename BaseTransportType

#define TRANSPORTSCAFFOLD TransportScaffold< \
	TransportType, \
	DelegateType, \
	BaseTransportType \
>

//---------------- Helper macros end ----------------//

template<TRANSPORTSCAFFOLD_TEMPLATE>
TRANSPORTSCAFFOLD::TransportScaffold(
	core::SocketAddress src_addr,
	core::SocketAddress dst_addr,
	BaseTransportType base_transport,
	core::TransportManager<TransportType>& transport_manager
) : src_addr(src_addr),
	dst_addr(dst_addr),
	base_transport(base_transport),
	transport_manager(transport_manager)
{}

template<TRANSPORTSCAFFOLD_TEMPLATE>
void TRANSPORTSCAFFOLD::setup(DelegateType* delegate) {
	this->delegate = delegate;
	base_transport.setup(static_cast<TransportType*>(this));
}

template<TRANSPORTSCAFFOLD_TEMPLATE>
template<typename... Args>
void TRANSPORTSCAFFOLD::did_dial(BaseTransportType, Args&&... args) {
	delegate->did_dial(static_cast<TransportType&>(*this), std::forward<Args>(args)...);
}

template<TRANSPORTSCAFFOLD_TEMPLATE>
void TRANSPORTSCAFFOLD::did_recv(BaseTransportType, BaseMessageType&& bytes) {
	delegate->did_recv(static_cast<TransportType&>(*this), std::move(bytes));
}

template<TRANSPORTSCAFFOLD_TEMPLATE>
void TRANSPORTSCAFFOLD::did_send(BaseTransportType, BaseMessageType&& bytes) {
	delegate->did_send(static_cast<TransportType&>(*this), std::move(bytes));
}

template<TRANSPORTSCAFFOLD_TEMPLATE>
void TRANSPORTSCAFFOLD::did_close(BaseTransportType, uint16_t reason) {
	delegate->did_close(static_cast<TransportType&>(*this), reason);
	transport_manager.erase(dst_addr);
}

template<TRANSPORTSCAFFOLD_TEMPLATE>
int TRANSPORTSCAFFOLD::send(MessageType&& buf) {
	return base_transport.send(std::move(buf));
}

template<TRANSPORTSCAFFOLD_TEMPLATE>
void TRANSPORTSCAFFOLD::close(uint16_t reason) {
	return base_transport.close(reason);
}

template<TRANSPORTSCAFFOLD_TEMPLATE>
bool TRANSPORTSCAFFOLD::is_internal() {
	return base_transport.is_internal();
}

//---------------- Helper macros undef begin ----------------//

#undef TRANSPORTSCAFFOLD_TEMPLATE
#undef TRANSPORTSCAFFOLD

//---------------- Helper macros undef end ----------------//

}  // namespace core
}  // namespace marlin
