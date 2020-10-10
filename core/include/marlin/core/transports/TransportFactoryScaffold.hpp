#ifndef MARLIN_CORE_TRANSPORTFACTORYSCAFFOLD
#define MARLIN_CORE_TRANSPORTFACTORYSCAFFOLD

#include <marlin/core/TransportManager.hpp>
#include <marlin/core/SocketAddress.hpp>

namespace marlin {
namespace core {

template<
	typename TransportFactoryType,
	typename TransportType,
	typename ListenDelegate,
	typename TransportDelegate,
	typename BaseTransportFactoryType,
	typename BaseTransportType
>
class TransportFactoryScaffold {
protected:
	BaseTransportFactoryType base_factory;

	ListenDelegate* delegate = nullptr;
	TransportManager<TransportType> transport_manager;

public:
	template<typename ...Args>
	TransportFactoryScaffold(Args&&... args);

	// BaseTransportFactoryType delegate
	bool should_accept(SocketAddress const& addr);
	void did_create_transport(BaseTransportType base_transport);

	core::SocketAddress addr;

	int bind(SocketAddress const& addr);
	int listen(ListenDelegate& delegate);
	int dial(SocketAddress const& addr, ListenDelegate& delegate);

	TransportType* get_transport(SocketAddress const& addr);
};

}  // namespace core
}  // namespace marlin

#include "TransportFactoryScaffold.ipp"

#endif  // MARLIN_CORE_TRANSPORTFACTORYSCAFFOLD
