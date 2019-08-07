#include "RlpxCrypto.hpp"

#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/hmac.h>
#include <cryptopp/modarith.h>
#include <cryptopp/oids.h>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

namespace marlin {
namespace rlpx {

void RlpxCrypto::generate_key() {
	static_private_key.Initialize(prng, CryptoPP::ASN1::secp256k1());
	static_private_key.MakePublicKey(static_public_key);
}

void RlpxCrypto::generate_ephemeral_key() {
	ephemeral_private_key.Initialize(prng, CryptoPP::ASN1::secp256k1());
	ephemeral_private_key.MakePublicKey(ephemeral_public_key);
}

void RlpxCrypto::store_key() {
	CryptoPP::FileSink fs("pkey.ec.der", true);
	static_private_key.Save(fs);
}

void RlpxCrypto::load_key() {
	CryptoPP::FileSource fs("pkey.ec.der", true);
	static_private_key.Load(fs);
	static_private_key.MakePublicKey(static_public_key);
}

void RlpxCrypto::log_pub_key() {
	std::string pubs;
	CryptoPP::StringSink ss(pubs);
	static_public_key.DEREncodePublicKey(ss);

	SPDLOG_INFO("Rlpx public key: {:spn}", spdlog::to_hex(pubs.substr(1)));
}

RlpxCrypto::RlpxCrypto() {
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

bool RlpxCrypto::ecies_decrypt(uint8_t *in, size_t in_size, uint8_t *out) {
	using namespace CryptoPP;

	{
		ECPPoint R(
			Integer(in + 3, 32),
			Integer(in + 35, 32)
		);

		ECP const &curve = static_private_key.GetGroupParameters().GetCurve();
		ECPPoint Pxy = curve.Multiply(static_private_key.GetPrivateExponent(), R);

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

		uint8_t digest[32];
		HMAC<SHA256> hmac(kMhash, 32);
		hmac.Update(in + 67, in_size - 99);
		hmac.Update(in, 2);
		hmac.TruncatedFinal(digest, 32);

		bool is_verified = (std::memcmp(in + in_size - 32, digest, 32) == 0);

		if(!is_verified) {
			return false;
		}

		CTR_Mode<AES>::Decryption d;
		d.SetKeyWithIV(kEM, 16, in + 67, 16);

		d.ProcessData(out, in + 83, in_size - 115);

		ECPPoint RSPK(
			Integer(out + 71, 32),
			Integer(out + 103, 32)
		);
		remote_static_public_key.Initialize(static_private_key.GetGroupParameters(), RSPK);
	}

	return true;
}

void RlpxCrypto::ecies_encrypt(uint8_t *in, size_t in_size, uint8_t *out) {
	using namespace CryptoPP;

	Integer r(prng, 32*8);
	ECP const &curve = static_private_key.GetGroupParameters().GetCurve();

	ECPPoint R = static_private_key.GetGroupParameters().ExponentiateBase(r);
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

void RlpxCrypto::get_static_public_key(uint8_t *out) {
	CryptoPP::ArraySink ssink(out, 65);
	static_public_key.DEREncodePublicKey(ssink);
}

void RlpxCrypto::get_ephemeral_public_key(uint8_t *out) {
	CryptoPP::ArraySink ssink(out, 65);
	ephemeral_public_key.DEREncodePublicKey(ssink);
}

void RlpxCrypto::get_nonce(uint8_t *out) {
	std::memcpy(out, nonce, 32);
}

void RlpxCrypto::compute_secrets(uint8_t *auth, uint8_t *authplain, size_t auth_size, uint8_t *ack, size_t ack_size) {
	using namespace CryptoPP;

	CryptoPP::ECDSA<
		CryptoPP::ECP,
		CryptoPP::SHA256
	>::PublicKey remote_ephemeral_public_key;

	{
		uint8_t *out = authplain + 83;

		uint8_t priv[32];
		static_private_key.GetPrivateExponent().Encode(priv, 32);

		ECDH<ECP>::Domain dh(ASN1::secp256k1());
		uint8_t sss[32];
		uint8_t temp = out[70];
		out[70] = 0x04;
		dh.Agree(sss, priv, out + 70);
		out[70] = temp;

		uint8_t e[32];
		for(int i = 0; i < 32; i++) {
			e[i] = sss[i] ^ out[136 + i];
		}
		Integer E(e, 32);

		uint8_t Rx[33];
		Rx[0] = 2 + out[68];
		std::memcpy(Rx+1, out+4, 32);

		ECP const &curve = static_private_key.GetGroupParameters().GetCurve();
		ECPPoint R;
		curve.DecodePoint(R, Rx, 33);

		Integer S(out + 36, 32);
		ModularArithmetic ma(static_private_key.GetGroupParameters().GetGroupOrder());
		Integer u1 = ma.Inverse(ma.Multiply(E, ma.MultiplicativeInverse(R.x)));
		Integer u2 = ma.Multiply(S, ma.MultiplicativeInverse(R.x));

		ECPPoint EPK = curve.Add(
			static_private_key.GetGroupParameters().ExponentiateBase(u1),
			curve.Multiply(u2, R)
		);

		remote_ephemeral_public_key.Initialize(static_private_key.GetGroupParameters(), EPK);
	}

	{
		uint8_t *out = authplain + 83;

		uint8_t priv[32];
		ephemeral_private_key.GetPrivateExponent().Encode(priv, 32);

		uint8_t pubs[65];
		CryptoPP::ArraySink ssink(pubs, 65);
		remote_ephemeral_public_key.DEREncodePublicKey(ssink);

		ECDH<ECP>::Domain dh(ASN1::secp256k1());
		uint8_t ek[32];
		dh.Agree(ek, priv, pubs);

		uint8_t digest[32];

		Keccak_256 keccak256;
		keccak256.Update(nonce, 32);
		keccak256.Update(out + 136, 32);
		keccak256.TruncatedFinal(digest, 32);

		keccak256.Restart();
		keccak256.Update(ek, 32);
		keccak256.Update(digest, 32);
		keccak256.TruncatedFinal(digest, 32);

		keccak256.Restart();
		keccak256.Update(ek, 32);
		keccak256.Update(digest, 32);
		keccak256.TruncatedFinal(aess, 32);
		SPDLOG_DEBUG("AESS: {:spn}", spdlog::to_hex(aess, aess + 32));

		keccak256.Restart();
		keccak256.Update(ek, 32);
		keccak256.Update(aess, 32);
		keccak256.TruncatedFinal(macs, 32);
		SPDLOG_DEBUG("MACS: {:spn}", spdlog::to_hex(macs, macs + 32));

		uint8_t iv[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		d.SetKeyWithIV(aess, 32, iv, 16);
		encryption.SetKeyWithIV(aess, 32, iv, 16);
	}

	{
		uint8_t *out = authplain + 83;

		uint8_t temp[32];

		for(int i = 0; i < 32; i++) {
			temp[i] = macs[i] ^ out[136 + i];
		}
		egress_mac.Update(temp, 32);
		egress_mac.Update(ack, ack_size);

		for(int i = 0; i < 32; i++) {
			temp[i] = macs[i] ^ nonce[i];
		}
		ingress_mac.Update(temp, 32);
		ingress_mac.Update(auth, auth_size);

		uint8_t digest[32];
		auto temp_mac = ingress_mac;
		temp_mac.TruncatedFinal(digest, 32);
		SPDLOG_DEBUG("IGD: {:spn}", spdlog::to_hex(digest, digest + 32));

		temp_mac = egress_mac;
		temp_mac.TruncatedFinal(digest, 32);
		SPDLOG_DEBUG("EGD: {:spn}", spdlog::to_hex(digest, digest + 32));
	}
}

bool RlpxCrypto::header_decrypt(uint8_t *in, size_t, uint8_t *out) {
	using namespace CryptoPP;

	uint8_t digest[16];
	auto temp_mac = ingress_mac;
	temp_mac.TruncatedFinal(digest, 16);

	ECB_Mode<AES>::Encryption e;
	e.SetKey(macs, 32);
	e.ProcessData(digest, digest, 16);

	SPDLOG_DEBUG("AES: {:spn}", spdlog::to_hex(digest, digest + 16));

	for(int i = 0; i < 16; i++) {
		digest[i] = digest[i] ^ in[i];
	}

	SPDLOG_DEBUG("XOR: {:spn}", spdlog::to_hex(digest, digest + 16));

	ingress_mac.Update(digest, 16);
	temp_mac = ingress_mac;
	temp_mac.TruncatedFinal(digest, 16);

	SPDLOG_DEBUG("MAC: {:spn}", spdlog::to_hex(digest, digest + 16));

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

	SPDLOG_DEBUG("AES: {:spn}", spdlog::to_hex(digest, digest + 16));

	for(int i = 0; i < 16; i++) {
		digest[i] = digest[i] ^ out[i];
	}

	SPDLOG_DEBUG("XOR: {:spn}", spdlog::to_hex(digest, digest + 16));

	egress_mac.Update(digest, 16);
	temp_mac = egress_mac;
	temp_mac.TruncatedFinal(digest, 16);

	SPDLOG_DEBUG("MAC: {:spn}", spdlog::to_hex(digest, digest + 16));

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

	SPDLOG_DEBUG("AES: {:spn}", spdlog::to_hex(digest, digest + 16));

	for(int i = 0; i < 16; i++) {
		digest[i] = digest[i] ^ temp[i];
	}

	SPDLOG_DEBUG("XOR: {:spn}", spdlog::to_hex(digest, digest + 16));

	ingress_mac.Update(digest, 16);
	temp_mac = ingress_mac;
	temp_mac.TruncatedFinal(digest, 16);

	SPDLOG_DEBUG("MAC: {:spn}", spdlog::to_hex(digest, digest + 16));

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

	SPDLOG_DEBUG("AES: {:spn}", spdlog::to_hex(digest, digest + 16));

	for(int i = 0; i < 16; i++) {
		digest[i] = digest[i] ^ temp[i];
	}

	SPDLOG_DEBUG("XOR: {:spn}", spdlog::to_hex(digest, digest + 16));

	egress_mac.Update(digest, 16);
	temp_mac = egress_mac;
	temp_mac.TruncatedFinal(digest, 16);

	SPDLOG_DEBUG("MAC: {:spn}", spdlog::to_hex(digest, digest + 16));

	std::memcpy(out + in_size - 16, digest, 16);

	return true;
}

} // namespace rlpx
} // namespace marlin
