#ifndef MARLIN_NEAR_NEARTRANSPORT_HPP
#define MARLIN_NEAR_NEARTRANSPORT_HPP

// #include <spdlog/spdlog.h>
#include <marlin/asyncio/tcp/TcpTransport.hpp>
#include <cryptopp/sha.h>
#include <sodium.h>

#include <cryptopp/blake2.h>
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/hmac.h>
#include <cryptopp/modarith.h>
#include <cryptopp/oids.h>
#include <boost/filesystem.hpp>



namespace marlin {
namespace near {

template <typename DelegateType>
class NearTransport {
private:
	using Self = NearTransport <DelegateType>;
	using BaseTransport = asyncio::TcpTransport<Self>;
	BaseTransport &transport;
	enum struct State {
		Idle,
		LengthWait,
		MessageReading,
		Done
	};
	State state = State::Idle;
	core::TransportManager <Self> &transportManager;

	uint8_t *buf; // The limit is yet to be set.
	uint32_t buf_size = 0;
	uint32_t bytes_remaining = 0;
public:

	DelegateType *delegate;
	uint8_t* create_handshake(uint8_t *my_key, uint8_t *near_key);
	void did_recv(BaseTransport &transport, core::Buffer &&bytes);
	void did_dial(BaseTransport &transport);
	void did_send_bytes(BaseTransport &transport, core::Buffer &&bytes);
	void did_send(BaseTransport &transport, core::Buffer &&bytes);
	void did_close(BaseTransport &transport, uint16_t reason);
	void send(core::Buffer &&message);

	NearTransport(
		core::SocketAddress const &src_addr,
		core::SocketAddress const &dst_addr,
		BaseTransport &transport,
		core::TransportManager<Self> &transportManager
	);

	void setup(DelegateType *delegate);
	void reply_handshake();


	core::SocketAddress src_addr;
	core::SocketAddress dst_addr;
};

void copy(uint8_t *dest, uint8_t *src, int len, int &offset_buf) {
	std::memcpy(dest, src, len);
	offset_buf += len;
}

template<typename DelegateType>
uint8_t* NearTransport<DelegateType>::create_handshake(uint8_t *my_key, uint8_t *near_key) {

	uint8_t *buf = new uint8_t[219];
	int offset_buf = 0;
	copy(buf + offset_buf, new uint8_t[13] {215, 0, 0, 0, 16, 34, 0, 0, 0, 33, 0, 0, 0}, 13, offset_buf);
	copy(buf + offset_buf, new uint8_t[1] {0}, 1, offset_buf);
	copy(buf + offset_buf, my_key, 32, offset_buf);
	copy(buf + offset_buf, new uint8_t[1] {0}, 1, offset_buf);
	copy(buf + offset_buf, near_key, 32, offset_buf);
	copy(buf + offset_buf, new uint8_t[67] {1, 247, 95, 16, 0, 0, 0, 116, 101, 115, 116, 45, 99, 104, 97, 105, 110, 45, 104, 98, 48, 54, 102, 130, 55, 103, 29, 8, 210, 56, 7, 20, 237, 123, 169, 71, 233, 37, 185, 242, 142, 22, 195, 56, 55, 91, 44, 115, 89, 101, 19, 3, 170, 163, 55, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 67, offset_buf);


	long long nonce = 1;
	if(boost::filesystem::exists("./.marlin/prev_nonce/nonce")) {
		std::ifstream sk("./.marlin/prev_nonce/nonce", std::ios::binary);
		char nonce_char[18];
		if(!sk.read(nonce_char, 18)) {
			throw;
		}
		nonce = std::stoll(nonce_char);
		nonce += 2;
		std::string nonce_str = std::to_string(nonce);
		while(nonce_str.size() < 18) {
			nonce_str = "0" + nonce_str;
		}
		std::ofstream sk1("./.marlin/prev_nonce/nonce", std::ios::binary);
		sk1.write(nonce_str.c_str(), 18);
	} else {
		boost::filesystem::create_directories("./.marlin/prev_nonce/");
		std::ofstream sk("./.marlin/prev_nonce/nonce", std::ios::binary);
		std::string nonce_str = std::to_string(nonce);
		while(nonce_str.size() < 18) {
			nonce_str = "0" + nonce_str;
		}
		sk.write(nonce_str.c_str(), 18);
	}
	// std::cout << nonce << '\n';
	nonce = 1;
	SPDLOG_INFO("{}", nonce);
	uint8_t nonce_arr[8];
	for(int i = 0; i < 8; i++) {
		nonce_arr[i] = nonce % 256;
		nonce /= 256;
	}
	copy(buf + offset_buf, nonce_arr, 8, offset_buf);

	// Creating the signature
	using namespace CryptoPP;
	CryptoPP::SHA256 sha256;
	uint8_t hashed_message[32];
	int my_key_offset = 13, near_key_offset = 46, nonce_offset = offset_buf - 8;

	int flag = std::memcmp(buf + my_key_offset, buf + near_key_offset, 33);
	if(flag < 0) {
		sha256.Update(buf + my_key_offset, 33);
		sha256.Update(buf + near_key_offset, 33);
	} else {
		sha256.Update(buf + near_key_offset, 33);
		sha256.Update(buf + my_key_offset, 33);
	}
	sha256.Update(buf + nonce_offset, 8);
	sha256.TruncatedFinal(hashed_message, 32);

	uint8_t my_signature[64];
	crypto_sign_detached(my_signature, NULL, hashed_message, 32, delegate->static_sk);
	copy(buf + offset_buf, new uint8_t[1] {0}, 1, offset_buf);
	copy(buf + offset_buf, my_signature, 64, offset_buf);
	SPDLOG_INFO("{}", offset_buf);
	return buf;
}

template<typename DelegateType>
void NearTransport<DelegateType>::did_dial(BaseTransport &) {
	SPDLOG_INFO(
		"{} {}",
		spdlog::to_hex(delegate->near_key, delegate->near_key + 64),
		delegate->near_addr
	);

	uint8_t *my_key = delegate->static_pk, *near_key = delegate->near_key;
	uint8_t *msg = new uint8_t[219];

	msg = this->create_handshake(my_key, near_key);

	core::Buffer buffer(msg, 219);
	SPDLOG_INFO("dsf sfksf sfdds {}", spdlog::to_hex(msg, msg + 219));

	transport.send(std::move(buffer));

}

template<typename DelegateType>
void NearTransport<DelegateType>::did_send_bytes(BaseTransport &, core::Buffer &&) {

}

template<typename DelegateType>
void NearTransport<DelegateType>::did_send(BaseTransport &, core::Buffer &&) {

}


template<typename DelegateType>
void NearTransport<DelegateType>::did_close(BaseTransport &, uint16_t reason) {
	delete[] buf;
	delegate->did_close(*this, reason);
	transportManager.erase(dst_addr);
}

template<typename DelegateType>
NearTransport<DelegateType>::NearTransport(
	core::SocketAddress const &src_addr,
	core::SocketAddress const &dst_addr,
	BaseTransport &transport,
	core::TransportManager<Self> &transportManager
): transport(transport), transportManager(transportManager), src_addr(src_addr), dst_addr(dst_addr) {
	SPDLOG_INFO("{}, {}", dst_addr.to_string(), src_addr.to_string());
}

template<typename DelegateType>
void NearTransport<DelegateType>::setup(
	DelegateType *delegate
) {
	this->delegate = delegate;

	transport.setup(this);
}

template<typename DelegateType>
void NearTransport<DelegateType>::send(
	core::Buffer &&message
) {
	transport.send(std::move(message));
}


template <typename DelegateType>
void NearTransport<DelegateType>::did_recv(
	BaseTransport&,
	core::Buffer &&bytes
) {
	// SPDLOG_DEBUG("{}", spdlog::to_hex(bytes.data(), bytes.data() + bytes.size()));
	if(bytes_remaining > bytes.size()) { // Partial message
		SPDLOG_DEBUG("Partial: {}, {}, {}", buf_size, bytes.size(), bytes_remaining);
		std::memcpy(buf + buf_size, bytes.data(), bytes.size());
		buf_size += bytes.size();
		bytes_remaining -= bytes.size();
		bytes.cover_unsafe(bytes.size());
	} else { // Full message
		SPDLOG_DEBUG("Full: {}, {}, {}, {}", buf_size, bytes.size(), bytes_remaining, state);
		std::memcpy(buf + buf_size, bytes.data(), bytes_remaining);
		buf_size += bytes_remaining;
		bytes.cover_unsafe(bytes_remaining);
		bytes_remaining = 0;

		if(state == State::Idle) {
			bytes_remaining = 4;
			buf = new uint8_t[4];
			state = State::LengthWait;
		} else if(state == State::LengthWait) {
			bytes_remaining = 0;
			for(int i = 3; i >= 0; i--) {
				bytes_remaining = buf[i] + (bytes_remaining << 8);
			}
			uint8_t *size = buf;
			buf = new uint8_t[bytes_remaining + 4];
			memcpy(buf, size, 4);
			delete[] size;
			state = State::MessageReading;
		} else if(state == State::MessageReading) {
			state = State::Done;
			core::Buffer message(buf, buf_size);
			buf = nullptr;
			message.cover_unsafe(4);
			delegate->did_recv(*this, std::move(message));
			state = State::Idle;
			bytes_remaining = 0;
			buf_size = 0;
		}

		if(bytes.size() > 0) {
			did_recv(transport, std::move(bytes));
		}
	}
}

} // near
} // marlin

#endif // MARLIN_NEAR_NEARTRANSPORT_HPP
