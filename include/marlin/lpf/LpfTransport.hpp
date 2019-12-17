#ifndef MARLIN_LPF_LPFTRANSPORT_HPP
#define MARLIN_LPF_LPFTRANSPORT_HPP

#include <list>
#include <unordered_set>
#include <spdlog/spdlog.h>

#include <marlin/net/SocketAddress.hpp>
#include <marlin/net/Buffer.hpp>
#include <marlin/net/core/TransportManager.hpp>

#include <marlin/lpf/CutThroughBuffer.hpp>
#include <marlin/lpf/StoreThenForwardBuffer.hpp>

#include <type_traits>

namespace marlin {
namespace lpf {

template<typename T>
struct IsTransportEncrypted {
	constexpr static bool value = false;
};

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through = false,
	int prefix_length = 8
>
class LpfTransport {
private:
	using Self = LpfTransport<
		DelegateType,
		StreamTransportType,
		should_cut_through,
		prefix_length
	>;
	using BaseTransport = StreamTransportType<Self>;

	BaseTransport &transport;
	net::TransportManager<Self> &transport_manager;

	std::unordered_map<uint16_t, StoreThenForwardBuffer> stf_buffers;
public:
	void did_recv_stf_message(uint16_t id, net::Buffer &&message);

	// Delegate
	void did_dial(BaseTransport &transport);
	void did_recv_bytes(BaseTransport &transport, net::Buffer &&bytes, uint16_t stream_id = 0);
	void did_send_bytes(BaseTransport &transport, net::Buffer &&bytes);
	void did_close(BaseTransport& transport);
	void did_recv_flush_stream(BaseTransport &transport, uint16_t id, uint64_t offset, uint64_t old_offset);

	net::SocketAddress src_addr;
	net::SocketAddress dst_addr;

	DelegateType *delegate = nullptr;

	LpfTransport(
		net::SocketAddress const &src_addr,
		net::SocketAddress const &dst_addr,
		BaseTransport &transport,
		net::TransportManager<Self> &transport_manager
	);

	void setup(DelegateType *delegate, uint8_t const* keys = nullptr);

	int send(net::Buffer &&message);
	void close();

	bool is_active();
	double get_rtt();

	int cut_through_send(net::Buffer &&message);
private:
	std::unordered_map<uint16_t, CutThroughBuffer> cut_through_buffers;
	std::list<uint16_t> cut_through_reserve_ids = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
public:
	std::unordered_set<uint16_t> cut_through_used_ids;
	uint16_t cut_through_send_start(uint64_t length);
	int cut_through_send_bytes(uint16_t id, net::Buffer &&bytes);
	void cut_through_send_end(uint16_t id);
	void cut_through_send_reset(uint16_t id);

	void cut_through_recv_start(uint16_t id, uint64_t length);
	void cut_through_recv_bytes(uint16_t id, net::Buffer &&bytes);
	void cut_through_recv_end(uint16_t id);
	void cut_through_recv_reset(uint16_t id);

	uint8_t const* get_static_pk();
	uint8_t const* get_remote_static_pk();
};


// Impl

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::did_recv_stf_message(
	uint16_t,
	net::Buffer &&message
) {
	delegate->did_recv_message(*this, std::move(message));
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::did_dial(
	BaseTransport &
) {
	delegate->did_dial(*this);
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::did_recv_bytes(
	BaseTransport &,
	net::Buffer &&bytes,
	uint16_t stream_id
) {
	if constexpr (should_cut_through) {
		if(should_cut_through && stream_id >= 10 && stream_id < 20) {
			auto &rbuf = cut_through_buffers[stream_id];

			if(rbuf.id == 0) { // New buf
				rbuf.id = stream_id;
			}

			int res = rbuf.did_recv_bytes(*this, std::move(bytes));

			if(res < 0) {
				close();
			}

			return;
		}
	}

	auto &stfbuf = stf_buffers[stream_id];
	if(stfbuf.id == 0) { // New buf
		stfbuf.id = stream_id;
	}

	int res = stfbuf.did_recv_bytes(*this, std::move(bytes));

	if(res < 0) {
		close();
	}
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::did_send_bytes(
	BaseTransport &,
	net::Buffer &&bytes
) {
	bytes.cover(8);
	delegate->did_send_message(*this, std::move(bytes));
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::did_close(
	BaseTransport&
) {
	delegate->did_close(*this);
	transport_manager.erase(dst_addr);
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::did_recv_flush_stream(BaseTransport &, uint16_t id, uint64_t, uint64_t) {
	cut_through_recv_reset(id);
}


template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::LpfTransport(
	net::SocketAddress const &src_addr,
	net::SocketAddress const &dst_addr,
	BaseTransport &transport,
	net::TransportManager<Self> &transport_manager
) : transport(transport), transport_manager(transport_manager), src_addr(src_addr), dst_addr(dst_addr), delegate(nullptr) {}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::setup(
	DelegateType *delegate,
	uint8_t const* keys
) {
	this->delegate = delegate;

	if constexpr (IsTransportEncrypted<StreamTransportType<DelegateType>>::value) {
		transport.setup(this, keys);
	} else {
		transport.setup(this);
	}
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
int LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::send(
	net::Buffer &&message
) {
	net::Buffer lpf_message(new char[message.size() + 8], message.size() + 8);

	lpf_message.write_uint64_be(0, message.size());
	std::memcpy(lpf_message.data() + 8, message.data(), message.size());

	return transport.send(std::move(lpf_message));
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
int LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::cut_through_send(
	net::Buffer &&message
) {
	auto id = cut_through_send_start(message.size());
	if(id == 0) {
		return send(std::move(message));
	}

	auto res = cut_through_send_bytes(id, std::move(message));

	if(res < 0) {
		return res;
	}

	cut_through_send_end(id);

	return 0;
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::close() {
	transport.close();
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
bool LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::is_active() {
	return transport.is_active();
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
double LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::get_rtt() {
	return transport.get_rtt();
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
uint16_t LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::cut_through_send_start(uint64_t length) {
	auto id = cut_through_reserve_ids.front();
	cut_through_reserve_ids.pop_front();
	cut_through_used_ids.insert(id);

	net::Buffer m(new char[8], 8);
	m.write_uint64_be(0, length);
	auto res = transport.send(std::move(m), id);

	if(res < 0) return 0;

	return id;
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
int LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::cut_through_send_bytes(uint16_t id, net::Buffer &&bytes) {
	return transport.send(std::move(bytes), id);
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::cut_through_send_end(uint16_t id) {
	cut_through_used_ids.erase(id);
	cut_through_reserve_ids.push_back(id);
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::cut_through_send_reset(uint16_t id) {
	transport.flush_stream(id);
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::cut_through_recv_start(uint16_t id, uint64_t length) {
	delegate->cut_through_recv_start(*this, id, length);
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::cut_through_recv_bytes(uint16_t id, net::Buffer &&bytes) {
	delegate->cut_through_recv_bytes(*this, id, std::move(bytes));
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::cut_through_recv_end(uint16_t id) {
	delegate->cut_through_recv_end(*this, id);
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
void LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::cut_through_recv_reset(uint16_t id) {
	delegate->cut_through_recv_reset(*this, id);
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
uint8_t const* LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::get_static_pk() {
	return transport.get_static_pk();
}

template<
	typename DelegateType,
	template<typename> class StreamTransportType,
	bool should_cut_through,
	int prefix_length
>
uint8_t const* LpfTransport<
	DelegateType,
	StreamTransportType,
	should_cut_through,
	prefix_length
>::get_remote_static_pk() {
	return transport.get_remote_static_pk();
}

} // namespace lpf
} // namespace marlin

#endif // MARLIN_LPF_LPFTRANSPORT_HPP
