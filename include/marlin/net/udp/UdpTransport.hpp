#ifndef MARLIN_NET_UDPTRANSPORT_HPP
#define MARLIN_NET_UDPTRANSPORT_HPP

#include <marlin/net/Buffer.hpp>
#include <marlin/net/SocketAddress.hpp>
#include <marlin/net/core/TransportManager.hpp>
#include <uv.h>
#include <spdlog/spdlog.h>

namespace marlin {
namespace net {

template<typename DelegateType>
class UdpTransport {
private:
	uv_udp_t *socket = nullptr;
	TransportManager<UdpTransport<DelegateType>> &transport_manager;

	static void send_cb(
		uv_udp_send_t *req,
		int status
	);

	struct SendPayload {
		Buffer packet;
		UdpTransport<DelegateType> &transport;
	};
public:
	SocketAddress src_addr;
	SocketAddress dst_addr;

	DelegateType *delegate = nullptr;

	UdpTransport(
		SocketAddress const &src_addr,
		SocketAddress const &dst_addr,
		uv_udp_t *socket,
		TransportManager<UdpTransport<DelegateType>> &transport_manager
	);

	void setup(DelegateType *delegate);
	void did_recv_packet(Buffer &&packet);
	int send(Buffer &&packet);
	void close();
};


// Impl

template<typename DelegateType>
UdpTransport<DelegateType>::UdpTransport(
	SocketAddress const &_src_addr,
	SocketAddress const &_dst_addr,
	uv_udp_t *_socket,
	TransportManager<UdpTransport<DelegateType>> &transport_manager
) : socket(_socket), transport_manager(transport_manager),
	src_addr(_src_addr), dst_addr(_dst_addr), delegate(nullptr) {}

template<typename DelegateType>
void UdpTransport<DelegateType>::setup(DelegateType *delegate) {
	this->delegate = delegate;
}

template<typename DelegateType>
void UdpTransport<DelegateType>::did_recv_packet(Buffer &&packet) {
	delegate->did_recv_packet(*this, std::move(packet));
}

template<typename DelegateType>
void UdpTransport<DelegateType>::send_cb(
	uv_udp_send_t *req,
	int status
) {
	auto *data = (SendPayload *)req->data;

	if(status < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Send callback error: {}",
			data->transport.dst_addr.to_string(),
			status
		);
	} else {
		data->transport.delegate->did_send_packet(
			data->transport,
			std::move(data->packet)
		);
	}

	delete data;
	delete req;
}

template<typename DelegateType>
int UdpTransport<DelegateType>::send(Buffer &&packet) {
	uv_udp_send_t *req = new uv_udp_send_t();
	auto req_data = new SendPayload{std::move(packet), *this};
	req->data = req_data;

	auto buf = uv_buf_init(req_data->packet.data(), req_data->packet.size());
	int res = uv_udp_send(
		req,
		socket,
		&buf,
		1,
		reinterpret_cast<const sockaddr *>(&dst_addr),
		UdpTransport<DelegateType>::send_cb
	);

	if (res < 0) {
		SPDLOG_ERROR(
			"Net: Socket {}: Send error: {}, To: {}",
			src_addr.to_string(),
			res,
			dst_addr.to_string()
		);
		return res;
	}

	return 0;
}

template<typename DelegateType>
void UdpTransport<DelegateType>::close() {
	delegate->did_close(*this);
	transport_manager.erase(dst_addr);
}

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_UDPTRANSPORT_HPP
