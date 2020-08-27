#ifndef MARLIN_RLPX_RLPXCRYPTO_HPP
#define MARLIN_RLPX_RLPXCRYPTO_HPP

#include <cryptopp/eccrypto.h>
#include <cryptopp/keccak.h>
#include <cryptopp/osrng.h>
#include <cryptopp/sha.h>
#include <cryptopp/modes.h>

#include <secp256k1.h>

namespace marlin {
namespace rlpx {

class RlpxCrypto {
private:
	secp256k1_context* ctx = nullptr;

	uint8_t static_seckey[32];
	secp256k1_pubkey static_pubkey;

	uint8_t ephemeral_seckey[32];
	secp256k1_pubkey ephemeral_pubkey;

	CryptoPP::ECDSA<
		CryptoPP::ECP,
		CryptoPP::SHA256
	>::PublicKey remote_static_public_key;

	secp256k1_pubkey remote_static_pubkey;

	uint8_t nonce[32];
	uint8_t aess[32];
	uint8_t macs[32];

	CryptoPP::Keccak_256 ingress_mac;
	CryptoPP::Keccak_256 egress_mac;

	CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption d;
	CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption encryption;

	void generate_key();
	void generate_ephemeral_key();
	void store_key();
	void load_key();
	void log_pub_key();
public:
	RlpxCrypto();
	~RlpxCrypto();

	bool ecies_decrypt(uint8_t *in, size_t in_size, uint8_t *out);
	bool ecies_decrypt_old(uint8_t *in, size_t in_size, uint8_t *out);
	void ecies_encrypt(uint8_t *in, size_t in_size, uint8_t *out);
	void get_static_public_key(uint8_t *out);
	void get_ephemeral_public_key(uint8_t *out);
	void get_nonce(uint8_t *out);

	void compute_secrets(uint8_t *auth, uint8_t *authplain, size_t auth_size, uint8_t *ack, size_t ack_size);
	void compute_secrets_old(uint8_t *auth, uint8_t *authplain, size_t auth_size, uint8_t *ack, size_t ack_size);

	bool header_decrypt(uint8_t *in, size_t in_size, uint8_t *out);
	bool header_encrypt(uint8_t *in, size_t in_size, uint8_t *out);
	bool frame_decrypt(uint8_t *in, size_t in_size, uint8_t *out);
	bool frame_encrypt(uint8_t *in, size_t in_size, uint8_t *out);
};

} // namespace rlpx
} // namespace marlin

#endif // MARLIN_RLPX_RLPXCRYPTO_HPP
