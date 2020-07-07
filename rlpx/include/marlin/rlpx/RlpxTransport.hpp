#ifndef MARLIN_RLPX_RLPXTRANSPORT_HPP
#define MARLIN_RLPX_RLPXTRANSPORT_HPP

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <snappy.h>

#include <marlin/asyncio/tcp/TcpTransport.hpp>

#include "RlpxCrypto.hpp"
#include "RlpItem.hpp"

namespace marlin {
namespace rlpx {

template<typename DelegateType>
class RlpxTransport {
private:
	typedef asyncio::TcpTransport<RlpxTransport<DelegateType>> BaseTransport;
	BaseTransport &transport;

	// Crypto
	RlpxCrypto crypto;

	enum struct State {
		Idle,
		LengthWait,
		AuthWait,
		HelloHeaderWait,
		HelloFrameWait,
		RecvHeaderWait,
		RecvFrameWait
	};
	State state = State::Idle;

	uint32_t buf_size = 0;
	uint32_t bytes_remaining = 0;
	uint32_t length;
	uint8_t buf[20000000]; // 20 MB limit(should never hit for ETH)
public:
	// Delegate
	void did_dial(BaseTransport &transport);
	void did_recv_bytes(BaseTransport &transport, core::Buffer &&bytes);
	void did_send_bytes(BaseTransport &transport, core::Buffer &&bytes);
	void did_close(BaseTransport &transport, uint16_t reason);

	core::SocketAddress src_addr;
	core::SocketAddress dst_addr;

	DelegateType *delegate;

	RlpxTransport(
		core::SocketAddress const &src_addr,
		core::SocketAddress const &dst_addr,
		BaseTransport &transport
	);

	void setup(DelegateType *delegate);
	int send(core::Buffer &&bytes);
};


// Impl

//---------------- Delegate functions begin ----------------//

template<typename DelegateType>
void RlpxTransport<DelegateType>::did_dial(
	BaseTransport &
) {
	// TODO: Initiate handshake
}

template<typename DelegateType>
void RlpxTransport<DelegateType>::did_recv_bytes(
	BaseTransport &,
	core::Buffer &&bytes
) {
	if(bytes_remaining > bytes.size()) { // Partial message
		SPDLOG_DEBUG("Partial: {}, {}, {}", buf_size, bytes.size(), bytes_remaining);
		std::memcpy(buf + buf_size, bytes.data(), bytes.size());
		buf_size += bytes.size();
		bytes_remaining -= bytes.size();
	} else { // Full message
		SPDLOG_DEBUG("Full: {}, {}, {}", buf_size, bytes.size(), bytes_remaining);
		std::memcpy(buf + buf_size, bytes.data(), bytes_remaining);
		buf_size += bytes_remaining;
		bytes.cover_unsafe(bytes_remaining);
		bytes_remaining = 0;

		if(state == State::Idle) {
			if(bytes.size() == 307) {
				bytes_remaining = 307;
				state = State::AuthWait;
			} else {
				bytes_remaining = 2;
				state = State::LengthWait;
			}
		} else if(state == State::LengthWait) {
			bytes_remaining = ((uint32_t)buf[0] << 8) | (uint32_t)buf[1];
			state = State::AuthWait;
		} else if(state == State::AuthWait) {
			// Have full auth message
			uint8_t buf_backup[1500];
			std::memcpy(buf_backup, buf, buf_size);

			bool is_verified = buf_size == 307
								? crypto.ecies_decrypt_old(buf, buf_size, buf + 81)
								: crypto.ecies_decrypt(buf, buf_size, buf + 83);

			if(is_verified) {
				// Respond
				uint8_t resp_ptxt[102] = {0xf8, 100, 0xb8};
				crypto.get_ephemeral_public_key(resp_ptxt + 3);
				resp_ptxt[3] = 0x40;
				resp_ptxt[68] = 0xa0;
				crypto.get_nonce(resp_ptxt + 69);
				resp_ptxt[101] = 0x04;

				core::Buffer resp(217);
				resp.write_uint16_be_unsafe(0, 215);
				crypto.ecies_encrypt(resp_ptxt, 102, resp.data());

				buf_size == 307
				? crypto.compute_secrets_old(buf_backup, buf, buf_size, resp.data(), 217)
				: crypto.compute_secrets(buf_backup, buf, buf_size, resp.data(), 217);

				transport.send(std::move(resp));

				uint8_t *hello = new uint8_t[144];
				std::memset(hello, 0, 144);
				hello[2] = 91;
				hello[3] = 0xc2;
				hello[4] = 0x80;
				hello[5] = 0x80;

				crypto.header_encrypt(hello, 32, hello);

				hello[32] = 0x80;
				hello[33] = 0xf8;
				hello[34] = 88;
				hello[35] = 0x05;
				hello[36] = 0x8c;
				std::memcpy(hello + 37, "Marlin/Alpha", 12);
				std::memcpy(hello + 49, "\xc6\xc5\x83\x65\x74\x68\x3f\x80\xb8", 9);
				crypto.get_static_public_key(hello + 58);
				hello[58] = 0x40;

				crypto.frame_encrypt(hello + 32, 112, hello + 32);

				transport.send(core::Buffer(hello, 144));

				bytes_remaining = 32;
				buf_size = 0;
				state = State::HelloHeaderWait;
			} else {
				SPDLOG_ERROR("Verification failed");
				// TODO: Abort and close transport
			}
		} else if(state == State::HelloHeaderWait) {
			bool is_verified = crypto.header_decrypt(buf, 32, buf);
			SPDLOG_DEBUG("Hello Header: {}", spdlog::to_hex(buf, buf + 32));

			if(is_verified) {
				length = bytes_remaining = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | (uint32_t)buf[2];
				if(bytes_remaining % 16 != 0) {
					bytes_remaining += 16 - bytes_remaining % 16;
				}
				bytes_remaining += 16;
				buf_size = 0;
				state = State::HelloFrameWait;
			}
		} else if(state == State::HelloFrameWait) {
			bool is_verified = crypto.frame_decrypt(buf, buf_size, buf);
			SPDLOG_DEBUG("Hello Frame: {}", spdlog::to_hex(buf, buf + buf_size));

			if(is_verified) {
				std::string cl(buf + 5, buf + 5 + (uint8_t)buf[4] - 0x80);
				SPDLOG_DEBUG("Client: {}", cl);

				bytes_remaining = 32;
				buf_size = 0;
				state = State::RecvHeaderWait;
			} else {
				SPDLOG_ERROR("Verification failed");
				// TODO: Abort and close transport
			}
		} else if(state == State::RecvHeaderWait) {
			bool is_verified = crypto.header_decrypt(buf, 32, buf);
			SPDLOG_DEBUG("Recv Header: {}", spdlog::to_hex(buf, buf + 32));

			if(is_verified) {
				length = bytes_remaining = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | (uint32_t)buf[2];
				if(bytes_remaining % 16 != 0) {
					bytes_remaining += 16 - bytes_remaining % 16;
				}
				bytes_remaining += 16;
				buf_size = 0;
				state = State::RecvFrameWait;
			}
		} else if(state == State::RecvFrameWait) {
			bool is_verified = crypto.frame_decrypt(buf, buf_size, buf);
			SPDLOG_DEBUG("Recv Frame: {} bytes: {}", length, spdlog::to_hex(buf, buf + buf_size));

			if(is_verified) {
				size_t ulen = 0;
				snappy::GetUncompressedLength((char *)buf + 1, length - 1, &ulen);

				uint8_t *ubuf = new uint8_t[ulen + 1];
				ubuf[0] = buf[0];
				snappy::RawUncompress((char *)buf + 1, length - 1, (char *)ubuf + 1);

				core::Buffer message(ubuf, ulen + 1);

				SPDLOG_DEBUG("Message: {} bytes: {}", ulen, spdlog::to_hex(ubuf, ubuf + ulen + 1));

				if(message.data()[0] == 0x02) { // p2p Ping
					auto pong = RlpItem::from_list({});

					core::Buffer res(pong.enc_size + 1);
					res.data()[0] = 0x03;
					pong.encode(res.data() + 1);

					this->send(std::move(res));
				} else {
					delegate->did_recv_message(*this, std::move(message));
				}

				bytes_remaining = 32;
				buf_size = 0;
				state = State::RecvHeaderWait;
			} else {
				SPDLOG_ERROR("Verification failed");
				// TODO: Abort and close transport
			}
		}

		if(bytes.size() > 0) {
			did_recv_bytes(transport, std::move(bytes));
		}
	}
}

template<typename DelegateType>
void RlpxTransport<DelegateType>::did_send_bytes(
	BaseTransport &,
	core::Buffer &&
) {
	// TODO: Notify delegate
}

template<typename DelegateType>
void RlpxTransport<DelegateType>::did_close(
	BaseTransport &,
	uint16_t reason
) {
	delegate->did_close(*this, reason);
}

//---------------- Delegate functions end ----------------//


template<typename DelegateType>
RlpxTransport<DelegateType>::RlpxTransport(
	core::SocketAddress const &src_addr,
	core::SocketAddress const &dst_addr,
	BaseTransport &transport
) : transport(transport), src_addr(src_addr), dst_addr(dst_addr) {}

template<typename DelegateType>
void RlpxTransport<DelegateType>::setup(
	DelegateType *delegate
) {
	this->delegate = delegate;

	transport.setup(this);
}

template<typename DelegateType>
int RlpxTransport<DelegateType>::send(
	core::Buffer &&message
) {
	uint64_t length = 1 + snappy::MaxCompressedLength(message.size() - 1);
	if(length % 16 != 0) {
		length += 16 - length % 16;
	}

	char *encoded = new char[length + 48];
	std::memset(encoded, 0, length + 48);

	size_t complen;
	snappy::RawCompress((char*)message.data() + 1, message.size() - 1, encoded + 33, &complen);
	encoded[32] = message.data()[0];

	complen += 1;
	length = complen;
	if(length % 16 != 0) {
		length += 16 - length % 16;
	}
	length += 48;

	encoded[0] = (uint8_t)(complen >> 16);
	encoded[1] = (uint8_t)(complen >> 8);
	encoded[2] = (uint8_t)(complen);
	encoded[3] = (uint8_t)0xc2;
	encoded[4] = (uint8_t)0x80;
	encoded[5] = (uint8_t)0x80;

	crypto.header_encrypt((uint8_t *)encoded, 32, (uint8_t *)encoded);

	crypto.frame_encrypt((uint8_t *)encoded + 32, length - 32, (uint8_t *)encoded + 32);

	core::Buffer bytes((uint8_t *)encoded, length);
	return transport.send(std::move(bytes));
}

} // namespace rlpx
} // namespace marlin

#endif // MARLIN_RLPX_RLPXTRANSPORT_HPP
