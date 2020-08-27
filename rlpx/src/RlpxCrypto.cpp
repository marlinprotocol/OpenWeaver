#include <marlin/rlpx/RlpxCrypto.hpp>

#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/hmac.h>
#include <cryptopp/modarith.h>
#include <cryptopp/oids.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#include <secp256k1_recovery.h>

namespace marlin {
namespace rlpx {

void RlpxCrypto::generate_key() {
	// Generate a valid key pair
	do {
		CryptoPP::OS_GenerateRandomBlock(false, static_seckey, 32);
	} while(
		secp256k1_ec_seckey_verify(ctx, static_seckey) != 1 &&
		secp256k1_ec_pubkey_create(ctx, &static_pubkey, static_seckey) != 1
	);
}

void RlpxCrypto::generate_ephemeral_key() {
	ephemeral_private_key.Initialize(prng, CryptoPP::ASN1::secp256k1());
	ephemeral_private_key.MakePublicKey(ephemeral_public_key);

	// Generate a valid key pair
	do {
		CryptoPP::OS_GenerateRandomBlock(false, ephemeral_seckey, 32);
	} while(
		secp256k1_ec_seckey_verify(ctx, ephemeral_seckey) != 1 &&
		secp256k1_ec_pubkey_create(ctx, &ephemeral_pubkey, ephemeral_seckey) != 1
	);
}

void RlpxCrypto::store_key() {
	CryptoPP::FileSink fs("key.sec", true);
	uint num = 32;
	do {
		num = fs.Put(static_seckey + (32 - num), num);
	} while(num != 0);
}

void RlpxCrypto::load_key() {
	// Load seckey
	CryptoPP::FileSource fs("key.sec", true);
	uint num = 0;
	do {
		num += fs.Get(static_seckey + num, 32 - num);
	} while(num < 32);

	// Load pubkey
	if(secp256k1_ec_pubkey_create(ctx, &static_pubkey, static_seckey) != 1) {
		// Make a new keypair on error
		SPDLOG_ERROR("Invalid private key, creating a temporary");
		generate_key();
	}
}

void RlpxCrypto::log_pub_key() {
	uint8_t pubkey[65];
	size_t size = 65;
	secp256k1_ec_pubkey_serialize(ctx, pubkey, &size, &static_pubkey, SECP256K1_EC_UNCOMPRESSED);

	SPDLOG_INFO("Enode: enode://{:spn}@127.0.0.1:12121", spdlog::to_hex(pubkey+1, pubkey+65));
}

RlpxCrypto::RlpxCrypto() : GroupParameters(CryptoPP::ASN1::secp256k1()) {
	ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);

	try {
		load_key();
	} catch(CryptoPP::FileStore::OpenErr &e) {
		generate_key();
		store_key();
	}

	log_pub_key();
	generate_ephemeral_key();
	CryptoPP::OS_GenerateRandomBlock(false, nonce, 32);
}

RlpxCrypto::~RlpxCrypto() {
	secp256k1_context_destroy(ctx);
}

bool RlpxCrypto::ecies_decrypt(uint8_t *in, size_t in_size, uint8_t *out) {
	// in:
	// 0	2	length
	// 2	65	ecies pubkey (R)
	// 67	16	iv
	// 83	X	ciphertext
	// 83+X	16	mac

	// R
	secp256k1_pubkey pubkey;
	if(secp256k1_ec_pubkey_parse(ctx, &pubkey, in+2, 65) != 1) {
		return false;
	}

	// Px, Py
	if(secp256k1_ec_pubkey_tweak_mul(ctx, &pubkey, static_seckey) != 1) {
		return false;
	}

	// S
	uint8_t S[65];
	size_t size = 65;
	secp256k1_ec_pubkey_serialize(ctx, S, &size, &pubkey, SECP256K1_EC_UNCOMPRESSED);

	using namespace CryptoPP;

	// kE, kM
	SHA256 sha256;
	uint8_t c[4] = {0,0,0,1};
	sha256.Update(c, 4);
	sha256.Update(S+1, 32);
	sha256.Update(nullptr, 0);
	uint8_t kEM[32];
	sha256.TruncatedFinal(kEM, 32);

	// Verify hmac
	sha256.Update(kEM + 16, 16);
	uint8_t kMhash[32];
	sha256.TruncatedFinal(kMhash, 32);

	uint8_t digest[32];
	HMAC<SHA256> hmac(kMhash, 32);
	hmac.Update(in + 67, in_size - 99);
	hmac.Update(in, 2);
	hmac.TruncatedFinal(digest, 32);

	bool is_verified = (std::memcmp(in + in_size - 32, digest, 32) == 0);

	if(!is_verified) {
		return false;
	}

	// Decrypt
	CTR_Mode<AES>::Decryption d;
	d.SetKeyWithIV(kEM, 16, in + 67, 16);

	d.ProcessData(out, in + 83, in_size - 115);

	// out:
	// 0	1	0xf8
	// 1	1	length
	// 2	1	0xb8
	// 3	1	0x41
	// 4	65	remote eph sig
	// 69	1	0xb8
	// 70	1	0x40
	// 71	64	remote static pubkey
	// ...

	// Set remote_static_pubkey
	out[70] = 0x04;
	if(secp256k1_ec_pubkey_parse(ctx, &remote_static_pubkey, out+70, 65) != 1) {
		return false;
	}

	return true;
}

bool RlpxCrypto::ecies_decrypt_old(uint8_t *in, size_t in_size, uint8_t *out) {
	// in:
	// 0	65	ecies pubkey (R)
	// 65	16	iv
	// 81	X	ciphertext
	// 81+X	16	mac

	// R
	secp256k1_pubkey pubkey;
	if(secp256k1_ec_pubkey_parse(ctx, &pubkey, in, 65) != 1) {
		return false;
	}

	// Px, Py
	if(secp256k1_ec_pubkey_tweak_mul(ctx, &pubkey, static_seckey) != 1) {
		return false;
	}

	// S
	uint8_t S[65];
	size_t size = 65;
	secp256k1_ec_pubkey_serialize(ctx, S, &size, &pubkey, SECP256K1_EC_UNCOMPRESSED);

	using namespace CryptoPP;

	// kE, kM
	SHA256 sha256;
	uint8_t c[4] = {0,0,0,1};
	sha256.Update(c, 4);
	sha256.Update(S+1, 32);
	sha256.Update(nullptr, 0);
	uint8_t kEM[32];
	sha256.TruncatedFinal(kEM, 32);

	// Verify hmac
	sha256.Update(kEM + 16, 16);
	uint8_t kMhash[32];
	sha256.TruncatedFinal(kMhash, 32);

	uint8_t digest[32];
	HMAC<SHA256> hmac(kMhash, 32);
	hmac.Update(in + 65, in_size - 97);
	hmac.TruncatedFinal(digest, 32);

	bool is_verified = (std::memcmp(in + in_size - 32, digest, 32) == 0);

	if(!is_verified) {
		return false;
	}

	// Decrypt
	CTR_Mode<AES>::Decryption d;
	d.SetKeyWithIV(kEM, 16, in + 65, 16);

	d.ProcessData(out, in + 81, in_size - 113);

	// out:
	// 0	65	remote eph sig
	// 65	32	hash
	// 97	64	remote static pubkey
	// ...

	// Set remote_static_pubkey
	out[96] = 0x04;
	if(secp256k1_ec_pubkey_parse(ctx, &remote_static_pubkey, out+96, 65) != 1) {
		return false;
	}

	return true;
}

void RlpxCrypto::ecies_encrypt(uint8_t *in, size_t in_size, uint8_t *out) {
	// Generate a random key pair
	uint8_t seckey[32];
	secp256k1_pubkey pubkey;
	do {
		CryptoPP::OS_GenerateRandomBlock(false, seckey, 32);
	} while(
		secp256k1_ec_seckey_verify(ctx, seckey) != 1 &&
		secp256k1_ec_pubkey_create(ctx, &pubkey, seckey) != 1
	);

	// Serialize pubkey
	size_t size = 65;
	secp256k1_ec_pubkey_serialize(ctx, out+2, &size, &pubkey, SECP256K1_EC_UNCOMPRESSED);

	// Px, Py
	secp256k1_pubkey Pxy = remote_static_pubkey;
	if(secp256k1_ec_pubkey_tweak_mul(ctx, &Pxy, seckey) != 1) {
		return;
	}
	uint8_t S[65];
	size = 65;
	secp256k1_ec_pubkey_serialize(ctx, S, &size, &Pxy, SECP256K1_EC_UNCOMPRESSED);

	using namespace CryptoPP;

	SHA256 sha256;
	uint8_t c[4] = {0,0,0,1};
	sha256.Update(c, 4);
	sha256.Update(S+1, 32);
	sha256.Update(nullptr, 0);
	uint8_t kEM[32];
	sha256.TruncatedFinal(kEM, 32);

	sha256.Update(kEM + 16, 16);
	uint8_t kMhash[32];
	sha256.TruncatedFinal(kMhash, 32);

	OS_GenerateRandomBlock(false, out + 67, 16);

	CTR_Mode<AES>::Encryption e;
	e.SetKeyWithIV(kEM, 16, out + 67, 16);

	e.ProcessData(out + 83, in, in_size);

	HMAC<SHA256> hmac(kMhash, 32);
	hmac.Update(out + 67, in_size + 16);
	hmac.Update(out, 2);
	hmac.TruncatedFinal(out + 83 + in_size, 32);

	{
		Integer r(prng, 32*8);
		ECP const &curve = GroupParameters.GetCurve();

		ECPPoint R = GroupParameters.ExponentiateBase(r);
		out[2] = 0x04;
		R.x.Encode(out + 3, 32);
		R.y.Encode(out + 35, 32);

		ECPPoint Pxy = curve.Multiply(
			r,
			remote_static_public_key.GetPublicElement()
		);

		uint8_t S[32];
		Pxy.x.Encode(S, 32);

		SHA256 sha256;
		uint8_t c[4] = {0,0,0,1};
		sha256.Update(c, 4);
		sha256.Update(S, 32);
		sha256.Update(nullptr, 0);
		uint8_t kEM[32];
		sha256.TruncatedFinal(kEM, 32);

		sha256.Restart();
		sha256.Update(kEM + 16, 16);
		uint8_t kMhash[32];
		sha256.TruncatedFinal(kMhash, 32);

		OS_GenerateRandomBlock(false, out + 67, 16);

		CTR_Mode<AES>::Encryption e;
		e.SetKeyWithIV(kEM, 16, out + 67, 16);

		e.ProcessData(out + 83, in, in_size);

		HMAC<SHA256> hmac(kMhash, 32);
		hmac.Update(out + 67, in_size + 16);
		hmac.Update(out, 2);
		hmac.TruncatedFinal(out + 83 + in_size, 32);
	}
}

void RlpxCrypto::get_static_public_key(uint8_t *out) {
	size_t size = 65;
	secp256k1_ec_pubkey_serialize(ctx, out, &size, &static_pubkey, SECP256K1_EC_UNCOMPRESSED);
}

void RlpxCrypto::get_ephemeral_public_key(uint8_t *out) {
	CryptoPP::ArraySink ssink(out, 65);
	ephemeral_public_key.DEREncodePublicKey(ssink);

	size_t size = 65;
	secp256k1_ec_pubkey_serialize(ctx, out, &size, &ephemeral_pubkey, SECP256K1_EC_UNCOMPRESSED);
}

void RlpxCrypto::get_nonce(uint8_t *out) {
	std::memcpy(out, nonce, 32);
}

void RlpxCrypto::compute_secrets(uint8_t *auth, uint8_t *authplain, size_t auth_size, uint8_t *ack, size_t ack_size) {
	// authplain:
	// 0	2	length
	// 2	65	ecies pubkey (R)
	// 67	16	iv
	// 83	X	plaintext
	// 83+X	16	mac

	// plaintext:
	// 0	1	0xf8
	// 1	1	length
	// 2	1	0xb8
	// 3	1	0x41
	// 4	65	remote eph sig
	// 69	1	0xb8
	// 70	1	0x40
	// 71	64	remote static pubkey
	// 135	1	0xa0
	// 136	32	nonce

	// sss
	secp256k1_pubkey rspk = remote_static_pubkey;
	if(secp256k1_ec_pubkey_tweak_mul(ctx, &rspk, static_seckey) != 1) {
		return;
	}
	uint8_t sss[65];
	size_t size = 65;
	secp256k1_ec_pubkey_serialize(ctx, sss, &size, &rspk, SECP256K1_EC_UNCOMPRESSED);

	// remote_ephemeral_pubkey
	secp256k1_ecdsa_recoverable_signature sig;
	secp256k1_ecdsa_recoverable_signature_parse_compact(
		ctx,
		&sig,
		authplain + 87,
		authplain[151]
	);
	uint8_t m[32];
	for(int i = 0; i < 32; i++) {
		m[i] = sss[1+i] ^ authplain[219 + i];
	}
	secp256k1_pubkey repk;
	if(secp256k1_ecdsa_recover(ctx, &repk, &sig, m) != 1) {
		return;
	}

	// eph secret
	if(secp256k1_ec_pubkey_tweak_mul(ctx, &repk, ephemeral_seckey) != 1) {
		return;
	}
	uint8_t es[65];
	size = 65;
	secp256k1_ec_pubkey_serialize(ctx, es, &size, &repk, SECP256K1_EC_UNCOMPRESSED);

	using namespace CryptoPP;

	uint8_t digest[32];

	// hash(nonce || remote nonce)
	Keccak_256 keccak256;
	keccak256.Update(nonce, 32);
	keccak256.Update(authplain + 219, 32);
	keccak256.TruncatedFinal(digest, 32);

	// shared-secret
	keccak256.Update(es, 32);
	keccak256.Update(digest, 32);
	keccak256.TruncatedFinal(digest, 32);

	// aes secret
	keccak256.Update(es, 32);
	keccak256.Update(digest, 32);
	keccak256.TruncatedFinal(aess, 32);
	SPDLOG_INFO("AESS: {:spn}", spdlog::to_hex(aess, aess + 32));

	// mac secret
	keccak256.Update(es, 32);
	keccak256.Update(aess, 32);
	keccak256.TruncatedFinal(macs, 32);
	SPDLOG_INFO("MACS: {:spn}", spdlog::to_hex(macs, macs + 32));

	// inititalize enc/dec
	uint8_t iv[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	d.SetKeyWithIV(aess, 32, iv, 16);
	encryption.SetKeyWithIV(aess, 32, iv, 16);

	// egress mac
	uint8_t temp[32];
	for(int i = 0; i < 32; i++) {
		temp[i] = macs[i] ^ authplain[219 + i];
	}
	egress_mac.Update(temp, 32);
	egress_mac.Update(ack, ack_size);

	// ingress mac
	for(int i = 0; i < 32; i++) {
		temp[i] = macs[i] ^ nonce[i];
	}
	ingress_mac.Update(temp, 32);
	ingress_mac.Update(auth, auth_size);

	auto temp_mac = ingress_mac;
	temp_mac.TruncatedFinal(digest, 32);
	SPDLOG_INFO("IGD: {:spn}", spdlog::to_hex(digest, digest + 32));

	temp_mac = egress_mac;
	temp_mac.TruncatedFinal(digest, 32);
	SPDLOG_INFO("EGD: {:spn}", spdlog::to_hex(digest, digest + 32));
}

void RlpxCrypto::compute_secrets_old(uint8_t *auth, uint8_t *authplain, size_t auth_size, uint8_t *ack, size_t ack_size) {
	// authplain:
	// 0	65	ecies pubkey (R)
	// 65	16	iv
	// 81	X	plaintext
	// 81+X	16	mac

	// plaintext:
	// 0	65	remote eph sig
	// 65	32	hash
	// 97	64	remote static pubkey
	// 161	32	nonce

	// sss
	secp256k1_pubkey rspk = remote_static_pubkey;
	if(secp256k1_ec_pubkey_tweak_mul(ctx, &rspk, static_seckey) != 1) {
		return;
	}
	uint8_t sss[65];
	size_t size = 65;
	secp256k1_ec_pubkey_serialize(ctx, sss, &size, &rspk, SECP256K1_EC_UNCOMPRESSED);

	// remote_ephemeral_pubkey
	secp256k1_ecdsa_recoverable_signature sig;
	secp256k1_ecdsa_recoverable_signature_parse_compact(
		ctx,
		&sig,
		authplain + 81,
		authplain[145]
	);
	uint8_t m[32];
	for(int i = 0; i < 32; i++) {
		m[i] = sss[1+i] ^ authplain[242 + i];
	}
	secp256k1_pubkey repk;
	if(secp256k1_ecdsa_recover(ctx, &repk, &sig, m) != 1) {
		return;
	}

	// eph secret
	if(secp256k1_ec_pubkey_tweak_mul(ctx, &repk, ephemeral_seckey) != 1) {
		return;
	}
	uint8_t es[65];
	size = 65;
	secp256k1_ec_pubkey_serialize(ctx, es, &size, &repk, SECP256K1_EC_UNCOMPRESSED);

	using namespace CryptoPP;

	uint8_t digest[32];

	// hash(nonce || remote nonce)
	Keccak_256 keccak256;
	keccak256.Update(nonce, 32);
	keccak256.Update(authplain + 242, 32);
	keccak256.TruncatedFinal(digest, 32);

	// shared-secret
	keccak256.Update(es, 32);
	keccak256.Update(digest, 32);
	keccak256.TruncatedFinal(digest, 32);

	// aes secret
	keccak256.Update(es, 32);
	keccak256.Update(digest, 32);
	keccak256.TruncatedFinal(aess, 32);
	SPDLOG_INFO("AESS: {:spn}", spdlog::to_hex(aess, aess + 32));

	// mac secret
	keccak256.Update(es, 32);
	keccak256.Update(aess, 32);
	keccak256.TruncatedFinal(macs, 32);
	SPDLOG_INFO("MACS: {:spn}", spdlog::to_hex(macs, macs + 32));

	// inititalize enc/dec
	uint8_t iv[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	d.SetKeyWithIV(aess, 32, iv, 16);
	encryption.SetKeyWithIV(aess, 32, iv, 16);

	// egress mac
	uint8_t temp[32];
	for(int i = 0; i < 32; i++) {
		temp[i] = macs[i] ^ authplain[242 + i];
	}
	egress_mac.Update(temp, 32);
	egress_mac.Update(ack, ack_size);

	// ingress mac
	for(int i = 0; i < 32; i++) {
		temp[i] = macs[i] ^ nonce[i];
	}
	ingress_mac.Update(temp, 32);
	ingress_mac.Update(auth, auth_size);

	auto temp_mac = ingress_mac;
	temp_mac.TruncatedFinal(digest, 32);
	SPDLOG_INFO("IGD: {:spn}", spdlog::to_hex(digest, digest + 32));

	temp_mac = egress_mac;
	temp_mac.TruncatedFinal(digest, 32);
	SPDLOG_INFO("EGD: {:spn}", spdlog::to_hex(digest, digest + 32));
}

bool RlpxCrypto::header_decrypt(uint8_t *in, size_t, uint8_t *out) {
	using namespace CryptoPP;

	uint8_t digest[16];
	auto temp_mac = ingress_mac;
	temp_mac.TruncatedFinal(digest, 16);

	ECB_Mode<AES>::Encryption e;
	e.SetKey(macs, 32);
	e.ProcessData(digest, digest, 16);

	SPDLOG_INFO("AES: {:spn}", spdlog::to_hex(digest, digest + 16));

	for(int i = 0; i < 16; i++) {
		digest[i] = digest[i] ^ in[i];
	}

	SPDLOG_INFO("XOR: {:spn}", spdlog::to_hex(digest, digest + 16));

	ingress_mac.Update(digest, 16);
	temp_mac = ingress_mac;
	temp_mac.TruncatedFinal(digest, 16);

	SPDLOG_INFO("MAC: {:spn}", spdlog::to_hex(digest, digest + 16));

	if(std::memcmp(in + 16, digest, 16) != 0) {
		return false;
	}

	d.ProcessData(out, in, 16);

	return true;
}

bool RlpxCrypto::header_encrypt(uint8_t *in, size_t, uint8_t *out) {
	using namespace CryptoPP;

	encryption.ProcessData(out, in, 16);

	uint8_t digest[16];
	auto temp_mac = egress_mac;
	temp_mac.TruncatedFinal(digest, 16);

	ECB_Mode<AES>::Encryption e;
	e.SetKey(macs, 32);
	e.ProcessData(digest, digest, 16);

	SPDLOG_INFO("AES: {:spn}", spdlog::to_hex(digest, digest + 16));

	for(int i = 0; i < 16; i++) {
		digest[i] = digest[i] ^ out[i];
	}

	SPDLOG_INFO("XOR: {:spn}", spdlog::to_hex(digest, digest + 16));

	egress_mac.Update(digest, 16);
	temp_mac = egress_mac;
	temp_mac.TruncatedFinal(digest, 16);

	SPDLOG_INFO("MAC: {:spn}", spdlog::to_hex(digest, digest + 16));

	std::memcpy(out + 16, digest, 16);

	return true;
}

bool RlpxCrypto::frame_decrypt(uint8_t *in, size_t in_size, uint8_t *out) {
	using namespace CryptoPP;

	uint8_t temp[16];
	uint8_t digest[16];
	ingress_mac.Update(in, in_size - 16);
	auto temp_mac = ingress_mac;
	temp_mac.TruncatedFinal(temp, 16);

	ECB_Mode<AES>::Encryption e;
	e.SetKey(macs, 32);
	e.ProcessData(digest, temp, 16);

	SPDLOG_INFO("AES: {:spn}", spdlog::to_hex(digest, digest + 16));

	for(int i = 0; i < 16; i++) {
		digest[i] = digest[i] ^ temp[i];
	}

	SPDLOG_INFO("XOR: {:spn}", spdlog::to_hex(digest, digest + 16));

	ingress_mac.Update(digest, 16);
	temp_mac = ingress_mac;
	temp_mac.TruncatedFinal(digest, 16);

	SPDLOG_INFO("MAC: {:spn}", spdlog::to_hex(digest, digest + 16));

	if(std::memcmp(in + in_size - 16, digest, 16) != 0) {
		return false;
	}

	d.ProcessData(out, in, in_size - 16);

	return true;
}

bool RlpxCrypto::frame_encrypt(uint8_t *in, size_t in_size, uint8_t *out) {
	using namespace CryptoPP;

	encryption.ProcessData(out, in, in_size - 16);

	uint8_t temp[16];
	uint8_t digest[16];
	egress_mac.Update(out, in_size - 16);
	auto temp_mac = egress_mac;
	temp_mac.TruncatedFinal(temp, 16);

	ECB_Mode<AES>::Encryption e;
	e.SetKey(macs, 32);
	e.ProcessData(digest, temp, 16);

	SPDLOG_INFO("AES: {:spn}", spdlog::to_hex(digest, digest + 16));

	for(int i = 0; i < 16; i++) {
		digest[i] = digest[i] ^ temp[i];
	}

	SPDLOG_INFO("XOR: {:spn}", spdlog::to_hex(digest, digest + 16));

	egress_mac.Update(digest, 16);
	temp_mac = egress_mac;
	temp_mac.TruncatedFinal(digest, 16);

	SPDLOG_INFO("MAC: {:spn}", spdlog::to_hex(digest, digest + 16));

	std::memcpy(out + in_size - 16, digest, 16);

	return true;
}

} // namespace rlpx
} // namespace marlin
