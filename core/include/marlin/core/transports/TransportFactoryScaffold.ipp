namespace marlin {
namespace core {

//---------------- Helper macros begin ----------------//

#define TRANSPORTFACTORYSCAFFOLD_TEMPLATE \
	typename TransportFactoryType, \
	typename TransportType, \
	typename ListenDelegate, \
	typename TransportDelegate, \
	typename BaseTransportFactoryType, \
	typename BaseTransportType

#define TRANSPORTFACTORYSCAFFOLD TransportFactoryScaffold< \
	TransportFactoryType, \
	TransportType, \
	ListenDelegate, \
	TransportDelegate, \
	BaseTransportFactoryType, \
	BaseTransportType \
>

//---------------- Helper macros end ----------------//

template<TRANSPORTFACTORYSCAFFOLD_TEMPLATE>
template<typename... Args>
TRANSPORTFACTORYSCAFFOLD::TransportFactoryScaffold(Args&&... args) :
	base_factory(std::forward<Args>(args)...) {}

template<TRANSPORTFACTORYSCAFFOLD_TEMPLATE>
bool TRANSPORTFACTORYSCAFFOLD::should_accept(SocketAddress const& addr) {
	return delegate->should_accept(addr);
}

template<TRANSPORTFACTORYSCAFFOLD_TEMPLATE>
void TRANSPORTFACTORYSCAFFOLD::did_create_transport(BaseTransportType base_transport) {
	auto* transport = transport_manager.get_or_create(
		base_transport.dst_addr,
		base_transport.src_addr,
		base_transport.dst_addr,
		base_transport,
		transport_manager
	).first;
	delegate->did_create_transport(*transport);
}

template<TRANSPORTFACTORYSCAFFOLD_TEMPLATE>
int TRANSPORTFACTORYSCAFFOLD::bind(SocketAddress const& addr) {
	this->addr = addr;
	return base_factory.bind(addr);
}

template<TRANSPORTFACTORYSCAFFOLD_TEMPLATE>
int TRANSPORTFACTORYSCAFFOLD::listen(ListenDelegate& delegate) {
	this->delegate = &delegate;
	return base_factory.listen(*static_cast<TransportFactoryType*>(this));
}

template<TRANSPORTFACTORYSCAFFOLD_TEMPLATE>
template<typename... Args>
int TRANSPORTFACTORYSCAFFOLD::dial(SocketAddress const& addr, ListenDelegate& delegate, Args&&... args) {
	this->delegate = &delegate;
	return base_factory.dial(addr, *static_cast<TransportFactoryType*>(this), std::forward<Args>(args)...);
}

template<TRANSPORTFACTORYSCAFFOLD_TEMPLATE>
TransportType* TRANSPORTFACTORYSCAFFOLD::get_transport(SocketAddress const& addr) {
	return transport_manager.get(addr);
}

//---------------- Helper macros undef begin ----------------//

#undef TRANSPORTFACTORYSCAFFOLD_TEMPLATE
#undef TRANSPORTFACTORYSCAFFOLD

//---------------- Helper macros undef end ----------------//

}  // namespace core
}  // namespace marlin
