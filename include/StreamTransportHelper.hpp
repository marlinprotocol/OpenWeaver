#include "StreamTransport.hpp"

namespace marlin {
namespace stream {

template<
	typename TransportDelegate,
	template<typename> class DatagramTransport
>
class StreamTransportHelper {
private:
	// Disallow creating an instance of this object
	StreamTransportHelper() {}

	typedef 
		StreamTransport <
			TransportDelegate,
			DatagramTransport
		> BaseTransport;

	typedef std::unordered_set<BaseTransport *> TransportSet;



public:
	static BaseTransport* find_random_rtt_transport(TransportSet& transport_set);
	static BaseTransport* find_min_rtt_transport(TransportSet& transport_set);
	static BaseTransport* find_max_rtt_transport(TransportSet& transport_set);
	static bool check_tranport_in_set(BaseTransport&, TransportSet& transport_set);
};

// Pick the ones with rtt's -1 too to give them a chance
template< 
	typename TransportDelegate,
	template<typename> class DatagramTransport
>
typename StreamTransportHelper<TransportDelegate, DatagramTransport>::BaseTransport*
StreamTransportHelper<
	TransportDelegate,
	DatagramTransport
>::find_random_rtt_transport(TransportSet& transport_set) {
	
	BaseTransport* to_return = nullptr;
	
	int set_size = transport_set.size();

	if (set_size != 0) {
		int random_to_return = rand() % set_size;
		to_return = *(std::next(transport_set.begin(), random_to_return));
	}

	return to_return;
}

// Pick the ones with rtt's -1 too to give them a chance
template< 
	typename TransportDelegate,
	template<typename> class DatagramTransport
>
typename StreamTransportHelper<TransportDelegate, DatagramTransport>::BaseTransport* StreamTransportHelper<
	TransportDelegate,
	DatagramTransport
>::find_min_rtt_transport(TransportSet& transport_set) {
	BaseTransport* to_return = nullptr;
	for (auto* temp_transport : transport_set) {
		if (temp_transport->get_rtt() == -1)
			return temp_transport;
		if (to_return == nullptr || temp_transport->get_rtt() < to_return->get_rtt()) {
			to_return = temp_transport;
		}	
	}

	return to_return;
}

template< 
	typename TransportDelegate,
	template<typename> class DatagramTransport
>
typename StreamTransportHelper<TransportDelegate, DatagramTransport>::BaseTransport* StreamTransportHelper<
	TransportDelegate,
	DatagramTransport
>::find_max_rtt_transport(TransportSet& transport_set) {
	BaseTransport* to_return = nullptr;
	for (auto* temp_transport : transport_set) {
		if (temp_transport->get_rtt() == -1) continue;
		if (to_return == nullptr || temp_transport->get_rtt() > to_return->get_rtt()) {
			to_return = temp_transport;
		}	
	}

	return to_return;
}

template< 
	typename TransportDelegate,
	template<typename> class DatagramTransport
>
bool StreamTransportHelper<
	TransportDelegate,
	DatagramTransport
>::check_tranport_in_set(BaseTransport& base_transport, TransportSet& transport_set) {
	if (transport_set.find(&base_transport) == transport_set.end()) {
		return false;
	}

	return true;
}

} // namespace stream
} // namespace marlin