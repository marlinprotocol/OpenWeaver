#ifndef MARLIN_LPF_LPFTRANSPORT_HPP
#define MARLIN_LPF_LPFTRANSPORT_HPP

#include <spdlog/spdlog.h>

#include <marlin/net/SocketAddress.hpp>
#include <marlin/net/Buffer.hpp>
#include <marlin/net/core/TransportManager.hpp>


namespace marlin {
namespace lpf {

template<typename DelegateType, template<typename> class StreamTransportType, int prefix_length = 8>
class LpfTransport {
private:
	using Self = LpfTransport<DelegateType, StreamTransportType, prefix_length>;
	using BaseTransport = StreamTransportType<Self>;

	BaseTransport &transport;
	net::TransportManager<Self> &transport_manager;

	uint64_t length = 0;
	char *buf = nullptr;
	uint64_t size = 0;

public:
	// Delegate
	void did_dial(BaseTransport &transport);
	void did_recv_bytes(BaseTransport &transport, net::Buffer &&bytes);
	void did_send_bytes(BaseTransport &transport, net::Buffer &&bytes);
	void did_close(BaseTransport& transport);

	net::SocketAddress src_addr;
	net::SocketAddress dst_addr;

	DelegateType *delegate;

	LpfTransport(
		net::SocketAddress const &src_addr,
		net::SocketAddress const &dst_addr,
		BaseTransport &transport,
		net::TransportManager<Self> &transport_manager
	);

	void setup(DelegateType *delegate);
	int send(net::Buffer &&message);
	void close();
};


// Impl

template<typename DelegateType, template<typename> class StreamTransportType, int prefix_length>
void LpfTransport<DelegateType, StreamTransportType, prefix_length>::did_dial(
	BaseTransport &
) {
	delegate->did_dial(*this);
}

template<typename DelegateType, template<typename> class StreamTransportType, int prefix_length>
void LpfTransport<DelegateType, StreamTransportType, prefix_length>::did_recv_bytes(
	BaseTransport &,
	net::Buffer &&bytes
) {
	if(buf == nullptr) { // Read length
		if(bytes.size() + size < 8) { // Partial length
			for(size_t i = 0; i < bytes.size(); i++) {
				length = (length << 8) | bytes.data()[i];
			}
			size += bytes.size();
		} else { // Full length
			for(size_t i = 0; i < 8 - size; i++) {
				length = (length << 8) | bytes.data()[i];
			}
			bytes.cover(8 - size);

			if(length > 5000000) { // Abort on big message, DoS prevention
				close();
			}

			// Prepare to process message
			buf = new char[length];
			size = 0;

			// Process remaining bytes
			did_recv_bytes(transport, std::move(bytes));
		}
	} else { // Read message
		if(bytes.size() + size < length) { // Partial message
			std::memcpy(buf + size, bytes.data(), bytes.size());
			size += bytes.size();
		} else { // Full message
			std::memcpy(buf + size, bytes.data(), length - size);
			bytes.cover(length - size);

			delegate->did_recv_message(*this, net::Buffer(buf, length));

			// Prepare to process length
			buf = nullptr;
			size = 0;
			length = 0;

			// Process remaining bytes
			did_recv_bytes(transport, std::move(bytes));
		}
	}
}

template<typename DelegateType, template<typename> class StreamTransportType, int prefix_length>
void LpfTransport<DelegateType, StreamTransportType, prefix_length>::did_send_bytes(
	BaseTransport &,
	net::Buffer &&bytes
) {
	bytes.cover(8);
	delegate->did_send_message(*this, std::move(bytes));
}

template<typename DelegateType, template<typename> class StreamTransportType, int prefix_length>
void LpfTransport<DelegateType, StreamTransportType, prefix_length>::did_close(
	BaseTransport&
) {
	delete buf;
	delegate->did_close(*this);
}

template<typename DelegateType, template<typename> class StreamTransportType, int prefix_length>
LpfTransport<DelegateType, StreamTransportType, prefix_length>::LpfTransport(
	net::SocketAddress const &src_addr,
	net::SocketAddress const &dst_addr,
	BaseTransport &transport,
	net::TransportManager<LpfTransport<DelegateType, StreamTransportType, prefix_length>> &transport_manager
) : transport(transport), transport_manager(transport_manager), src_addr(src_addr), dst_addr(dst_addr) {}

template<typename DelegateType, template<typename> class StreamTransportType, int prefix_length>
void LpfTransport<DelegateType, StreamTransportType, prefix_length>::setup(
	DelegateType *delegate
) {
	this->delegate = delegate;

	transport.setup(this);
}

template<typename DelegateType, template<typename> class StreamTransportType, int prefix_length>
int LpfTransport<DelegateType, StreamTransportType, prefix_length>::send(
	net::Buffer &&message
) {
	net::Buffer lpf_message(new char[message.size() + 8], message.size() + 8);

	lpf_message.write_uint64_be(0, message.size());
	std::memcpy(lpf_message.data() + 8, message.data(), message.size());

	return transport.send(std::move(lpf_message));
}

template<typename DelegateType, template<typename> class StreamTransportType, int prefix_length>
void LpfTransport<DelegateType, StreamTransportType, prefix_length>::close() {
	transport.close();
	delegate->did_close(*this);
	transport_manager.erase(dst_addr);
}

} // namespace lpf
} // namespace marlin

#endif // MARLIN_LPF_LPFTRANSPORT_HPP
