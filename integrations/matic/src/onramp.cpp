#include "OnRamp.hpp"
#include <unistd.h>

#include <structopt/app.hpp>
#include <cryptopp/scrypt.h>
#include <cryptopp/modes.h>
#include <cryptopp/keccak.h>
#include <cryptopp/hex.h>
#include <cryptopp/aes.h>
#include <rapidjson/document.h>
#include <boost/filesystem.hpp>


using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::beacon;
using namespace marlin::pubsub;
using namespace marlin::rlpx;

struct CliOptions {
	std::optional<std::string> discovery_addr;
	std::optional<std::string> pubsub_addr;
	std::optional<std::string> beacon_addr;
	std::optional<std::string> keystore_path;
	std::optional<std::string> keystore_pass_path;
	enum class Contracts { mainnet, kovan };
	std::optional<Contracts> contracts;
	// std::optional<SpamCheckMode> spamcheck;
	std::optional<std::string> spamcheck_addr;
};
STRUCTOPT(CliOptions, discovery_addr, pubsub_addr, beacon_addr, keystore_path, keystore_pass_path, contracts, spamcheck_addr);

std::string get_key(std::string keystore_path, std::string keystore_pass_path);

int main(int argc, char** argv) {
	try {
		auto options = structopt::app("gateway").parse<CliOptions>(argc, argv);
		std::string key;
		if(options.beacon_addr.has_value()) {
			if(options.keystore_path.has_value() && options.keystore_pass_path.has_value()) {
				key = get_key(options.keystore_path.value(), options.keystore_pass_path.value());
				if (key.empty()) {
					SPDLOG_ERROR("keystore file error");
					return 1;
				}
			} else {
				SPDLOG_ERROR("require keystore and password file");
				return 1;
			}
		}
		auto discovery_addr = SocketAddress::from_string(
			options.discovery_addr.value_or("0.0.0.0:22702")
		);
		auto pubsub_addr = SocketAddress::from_string(
			options.pubsub_addr.value_or("0.0.0.0:22700")
		);
		auto beacon_addr = SocketAddress::from_string(
			options.beacon_addr.value_or("127.0.0.1:8002")
		);

		std::string staking_url;
		switch(options.contracts.value_or(CliOptions::Contracts::mainnet)) {
		case CliOptions::Contracts::mainnet:
			staking_url = "/subgraphs/name/marlinprotocol/staking";
			break;
		case CliOptions::Contracts::kovan:
			staking_url = "/subgraphs/name/marlinprotocol/staking-kovan";
			break;
		};

		SPDLOG_INFO(
			"Starting gateway with discovery: {}, pubsub: {}, beacon: {}, spamcheck: {}",
			discovery_addr.to_string(),
			pubsub_addr.to_string(),
			beacon_addr.to_string(),
			options.spamcheck_addr.value_or("none")
		);

		{
			if(key.size() == 0) {
				SPDLOG_INFO("Bridge: No identity key provided");
			} else if(key.size() == 32) {
				auto* ctx_signer = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
				auto* ctx_verifier = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
				if(secp256k1_ec_seckey_verify(ctx_verifier, (uint8_t*)key.c_str()) != 1) {
					SPDLOG_ERROR("Bridge: failed to verify key", key.size());
					secp256k1_context_destroy(ctx_verifier);
					secp256k1_context_destroy(ctx_signer);
					return -1;
				}

				secp256k1_pubkey pubkey;
				auto res = secp256k1_ec_pubkey_create(
					ctx_signer,
					&pubkey,
					(uint8_t*)key.c_str()
				);
				(void)res;

				uint8_t pubkeyser[65];
				size_t len = 65;
				secp256k1_ec_pubkey_serialize(
					ctx_signer,
					pubkeyser,
					&len,
					&pubkey,
					SECP256K1_EC_UNCOMPRESSED
				);

				// Get address
				uint8_t hash[32];
				CryptoPP::Keccak_256 hasher;
				hasher.CalculateTruncatedDigest(hash, 32, pubkeyser+1, 64);
				// address is in hash[12..31]

				SPDLOG_INFO("Bridge: Identity is 0x{:spn}", spdlog::to_hex(hash+12, hash+32));

				secp256k1_context_destroy(ctx_verifier);
				secp256k1_context_destroy(ctx_signer);
			} else {
				SPDLOG_ERROR("Bridge: failed to load key: {}", key.size());
			}
		}

		uint8_t static_sk[crypto_box_SECRETKEYBYTES];
		uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
		crypto_box_keypair(static_pk, static_sk);

		DefaultMulticastClientOptions clop {
			static_sk,
			static_pk,
			std::vector<uint16_t>({0, 1}),
			beacon_addr.to_string(),
			discovery_addr.to_string(),
			pubsub_addr.to_string(),
			staking_url,
			"0xa6a7de01e8b7ba6a4a61c782a73188d808fc1f3cf5743fadb68a02ed884b594"
		};

		OnRamp onramp(clop, options.spamcheck_addr, (uint8_t*)key.data());

		return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}

std::string string_to_hex(const std::string& input)
{
    std::string output;
	CryptoPP::StringSource ss2( input, true,
    new CryptoPP::HexEncoder(
        new CryptoPP::StringSink( output )
    )); // HexEncoder
    return output;
}

std::string hex_to_string(const std::string& input)
{
	std::string output;
	CryptoPP::StringSource ss2( input, true,
    new CryptoPP::HexDecoder(
        new CryptoPP::StringSink( output )
    )); // HexDecoder
    return output;
}

bool isvalid_cipherparams(const rapidjson::Value &cipherparams, const std::string &cipher) {
	if(cipher == "aes-128-ctr") {
		return cipherparams.HasMember("iv") && cipherparams["iv"].IsString();
	}
	return false;
}

bool isvalid_kdfparams(const rapidjson::Value &kdfparams, const std::string &kdf) {
	if(kdf == "scrypt") {
		return kdfparams.HasMember("dklen") && kdfparams["dklen"].IsUint64()
				&& kdfparams.HasMember("n") && kdfparams["n"].IsUint64()
				&& kdfparams.HasMember("p") && kdfparams["p"].IsUint64()
				&& kdfparams.HasMember("r") && kdfparams["r"].IsUint64()
				&& kdfparams.HasMember("salt") && kdfparams["salt"].IsString();
	}
	return false;
}

bool isvalid_keystore(rapidjson::Document &keystore) {
	if (keystore.HasMember("crypto") && keystore["crypto"].IsObject() ) {
		const rapidjson::Value &crypto = keystore["crypto"];
		const rapidjson::Value &cipherparams = crypto["cipherparams"];
		const rapidjson::Value &kdfparams = crypto["kdfparams"];
		return crypto.HasMember("cipher") && crypto["cipher"].IsString()
			&& crypto.HasMember("ciphertext") && crypto["ciphertext"].IsString()
			&& crypto.HasMember("kdf") && crypto["kdf"].IsString()
			&& crypto.HasMember("mac") && crypto["mac"].IsString()
			&& crypto.HasMember("cipherparams") && crypto["cipherparams"].IsObject()
			&& crypto.HasMember("kdfparams") && crypto["kdfparams"].IsObject()
			&& isvalid_cipherparams(cipherparams, crypto["cipher"].GetString())
			&& isvalid_kdfparams(kdfparams, crypto["kdf"].GetString());
	}
	return false;
}

void derivekey_scrypt(CryptoPP::SecByteBlock &derived, const rapidjson::Value &kdfparams, const std::string &pass) {

	CryptoPP::Scrypt pbkdf;
	derived = CryptoPP::SecByteBlock(kdfparams["dklen"].GetUint());
	std::string salt(hex_to_string(kdfparams["salt"].GetString()));
	pbkdf.DeriveKey(derived, derived.size(),
			CryptoPP::ConstBytePtr(pass), CryptoPP::BytePtrSize(pass),
			CryptoPP::ConstBytePtr(salt), CryptoPP::BytePtrSize(salt),
			CryptoPP::word64(kdfparams["n"].GetUint64()),
			CryptoPP::word64(kdfparams["r"].GetUint64()),
			CryptoPP::word64(kdfparams["p"].GetUint64()));
}

std::string decrypt_aes128ctr(const rapidjson::Value &cipherparams, std::string &ciphertext, CryptoPP::SecByteBlock &derived) {
	std::string iv = hex_to_string(cipherparams["iv"].GetString());

	CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption d;
	d.SetKeyWithIV(derived, 16, CryptoPP::ConstBytePtr(iv));

	CryptoPP::SecByteBlock decrypted(ciphertext.size());
	d.ProcessData(decrypted, CryptoPP::ConstBytePtr(ciphertext), CryptoPP::BytePtrSize(ciphertext));
	return std::string((const char*)decrypted.data(), decrypted.size());
}

bool check_mac(const std::string &mac, CryptoPP::SecByteBlock &derived, std::string &ciphertext) {
	CryptoPP::Keccak_256 hasher;
	auto hashinput = CryptoPP::SecByteBlock((derived.data()+16), 16) + CryptoPP::SecByteBlock((const CryptoPP::byte*)ciphertext.data(), ciphertext.size());

	CryptoPP::SecByteBlock hash(mac.size());
	return hasher.VerifyTruncatedDigest((const CryptoPP::byte*)mac.data(), mac.size(), hashinput, hashinput.size());
}

std::string get_key(std::string keystore_path, std::string keystore_pass_path) {
	std::string _pass;
	rapidjson::Document _keystore;

	// read password file
	if(boost::filesystem::exists(keystore_pass_path)) {
		std::ifstream fin(keystore_pass_path);
		std::getline(fin, _pass);
		fin.close();
	}
	if (_pass.empty()) {
		SPDLOG_ERROR("Invalid password file");
		return "";
	}

	// read keystore file
	if(boost::filesystem::exists(keystore_path)) {
		std::string s;
		boost::filesystem::load_string_file(keystore_path, s);
		rapidjson::StringStream ss(s.c_str());
		_keystore.ParseStream(ss);
	}
	if (!isvalid_keystore(_keystore)){
		SPDLOG_ERROR("Invalid keystore file");
		return "";
	}

	const rapidjson::Value &crypto = _keystore["crypto"];
	std::string ciphertext = hex_to_string(crypto["ciphertext"].GetString());
	CryptoPP::SecByteBlock derived;
	std::string decrypted;

	// get derived keycrypto
	if (crypto["kdf"] == "scrypt") {
		derivekey_scrypt(derived, crypto["kdfparams"], _pass);
	}
	if (derived.size() == 0) {
		SPDLOG_ERROR("kdf error");
		return "";
	}
	if (crypto["cipher"] == "aes-128-ctr") {
		decrypted  = decrypt_aes128ctr(crypto["cipherparams"], ciphertext, derived);
	}

	//validate mac
	if (!check_mac(hex_to_string(crypto["mac"].GetString()), derived, ciphertext)) {
		SPDLOG_ERROR("Invalid mac");
		return "";
	}
	SPDLOG_INFO("decrypted keystore");
	return decrypted;
}

