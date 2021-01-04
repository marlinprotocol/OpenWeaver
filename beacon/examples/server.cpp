#include <marlin/beacon/DiscoveryServer.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>
#include <cstring>
#include <structopt/app.hpp>
#include <cryptopp/scrypt.h>
#include <cryptopp/modes.h>
#include <cryptopp/keccak.h>
#include <cryptopp/hex.h>
#include <cryptopp/aes.h>
#include <cryptopp/osrng.h>
#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>
#include <boost/filesystem.hpp>
#include <secp256k1_recovery.h>

using namespace marlin;

class BeaconDelegate {};

struct CliOptions {
	std::optional<std::string> keystore_path;
	std::optional<std::string> keystore_pass_path;
	std::optional<std::string> discovery_addr;
	std::optional<std::string> heartbeat_addr;
	std::optional<std::string> beacon_addr;
};
STRUCTOPT(CliOptions,  keystore_path, keystore_pass_path, discovery_addr, heartbeat_addr, beacon_addr);

std::string get_key(std::string keystore_path, std::string keystore_pass_path);
std::string create_key(uint8_t keybytes, uint8_t ivbytes, const std::string &password);

int main(int argc, char** argv) {
	try {
		auto options = structopt::app("beacon").parse<CliOptions>(argc, argv);
		if(options.beacon_addr.has_value()) {
			std::string key;
			if(options.keystore_path.has_value() && options.keystore_pass_path.has_value()) {
				key = get_key(options.keystore_path.value(), options.keystore_pass_path.value());
			} else if(boost::filesystem::exists("./.marlin/keystore/key") && boost::filesystem::exists("./.marlin/keystore/pass")) {
				SPDLOG_INFO("found keystore and password file");
				key = get_key("./.marlin/keystore/key", "./.marlin/keystore/pass");
			} else {
				boost::filesystem::create_directories("./.marlin/keystore/");
				key = create_key(32, 16, "beacon_pass");
				SPDLOG_INFO("created keystore and password file");
			}

			if (key.empty()) {
				SPDLOG_ERROR("keystore file error");
				return 1;
			}
		}
		auto discovery_addr = core::SocketAddress::from_string(
			options.discovery_addr.value_or("127.0.0.1:8002")
		);
		auto heartbeat_addr = core::SocketAddress::from_string(
			options.heartbeat_addr.value_or("127.0.0.1:8003")
		);
		auto beacon_addr = options.beacon_addr.has_value() ? std::make_optional(core::SocketAddress::from_string(*options.beacon_addr)) : std::nullopt;

		SPDLOG_INFO(
			"Starting beacon with discovery: {}, heartbeat: {}",
			discovery_addr.to_string(),
			heartbeat_addr.to_string()
		);

		BeaconDelegate del;
		beacon::DiscoveryServer<BeaconDelegate> b(discovery_addr, heartbeat_addr, beacon_addr);
		b.delegate = &del;

		return asyncio::EventLoop::run();
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

std::string encrypt_aes128ctr(const std::string &iv, CryptoPP::SecByteBlock &derived, const std::string &key) {
	CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption e;
	e.SetKeyWithIV(derived, 16, CryptoPP::ConstBytePtr(iv));

	CryptoPP::SecByteBlock encrypted(key.size()); //nitin cipher text size = priv key size
	e.ProcessData(encrypted, CryptoPP::ConstBytePtr(key), CryptoPP::BytePtrSize(key));
	return std::string((const char*)encrypted.data(), encrypted.size());
}
bool check_mac(const std::string &mac, CryptoPP::SecByteBlock &derived, std::string &ciphertext) {
	CryptoPP::Keccak_256 hasher;
	auto hashinput = CryptoPP::SecByteBlock((derived.data()+16), 16) + CryptoPP::SecByteBlock((const CryptoPP::byte*)ciphertext.data(), ciphertext.size());
	
	CryptoPP::SecByteBlock hash(mac.size());
	return hasher.VerifyTruncatedDigest((const CryptoPP::byte*)mac.data(), mac.size(), hashinput, hashinput.size());
}

std::string get_mac(CryptoPP::SecByteBlock &derived, std::string &ciphertext, uint8_t hashsize) {
	CryptoPP::Keccak_256 hasher;
	auto hashinput = CryptoPP::SecByteBlock((derived.data()+16), 16) + CryptoPP::SecByteBlock((const CryptoPP::byte*)ciphertext.data(), ciphertext.size());
	
	CryptoPP::SecByteBlock hash(hashsize);
	hasher.CalculateTruncatedDigest(hash, hashsize, hashinput, hashinput.size());
	return std::string((const char*)hash.data(), hash.size());
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

std::string create_key(uint8_t keybytes, uint8_t ivbytes, const std::string &password) {
	
	std::string random_bytes;
	random_bytes.resize(keybytes + ivbytes + keybytes);

	secp256k1_context* ctx_verifier = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
	do {
		CryptoPP::OS_GenerateRandomBlock(false, (unsigned char*)random_bytes.c_str(), keybytes + ivbytes + keybytes);
	} while(
		secp256k1_ec_seckey_verify(ctx_verifier, (unsigned char*)random_bytes.c_str()) != 1
	);

	std::string keyData = R"({
		"version": 3,
		"crypto": {
			"ciphertext": "07533e172414bfa50e99dba4a0ce603f654ebfa1ff46277c3e0c577fdc87f6bb4e4fe16c5a94ce6ce14cfa069821ef9b",
			"cipherparams": { "iv": "16d67ba0ce5a339ff2f07951253e6ba8" },
			"kdf": "scrypt",
			"kdfparams": { "dklen": 32,  "n": 262144,  "r": 8,  "p": 1,  "salt": "06870e5e6a24e183a5c807bd1c43afd86d573f7db303ff4853d135cd0fd3fe91" },
			"mac": "8ccded24da2e99a11d48cda146f9cc8213eb423e2ea0d8427f41c3be414424dd",
			"cipher": "aes-128-ctr"
		},
		"address": "ea38373371b5d60298aa0fa035c2b592b5af98eb",
		"id": "0498f19a-59db-4d54-ac95-33901b4f1870"
	})";

	rapidjson::Document keystore;
	keystore.Parse(keyData.c_str());

	rapidjson::Value &dklen = keystore["crypto"]["kdfparams"]["dklen"];
	dklen.SetInt(keybytes);

	rapidjson::Value &salt = keystore["crypto"]["kdfparams"]["salt"];
	std::string salthex = string_to_hex(random_bytes.substr(keybytes + ivbytes));
	salt.SetString(salthex.c_str(), salthex.size());

	rapidjson::Value &iv = keystore["crypto"]["cipherparams"]["iv"];
	std::string ivhex = string_to_hex(random_bytes.substr(keybytes, ivbytes));
	iv.SetString(ivhex.c_str(), ivhex.size());

	CryptoPP::SecByteBlock derived;
	derivekey_scrypt(derived, keystore["crypto"]["kdfparams"], password);

	rapidjson::Value &ciphertext = keystore["crypto"]["ciphertext"];
	std::string encrypted = encrypt_aes128ctr(random_bytes.substr(keybytes, ivbytes), derived, random_bytes.substr(0, keybytes));
	std::string encryptedhex = string_to_hex(encrypted);
	ciphertext.SetString(encryptedhex.c_str(), encryptedhex.size());
	
	rapidjson::Value &mac = keystore["crypto"]["mac"];
	std::string machex = string_to_hex(get_mac(derived, encrypted, 32));
	mac.SetString(machex.c_str(), machex.size());

	std::ofstream ofs_pass("./.marlin/keystore/pass");
	ofs_pass.write(password.c_str(), password.size());

	FILE* fp = fopen("./.marlin/keystore/key", "w");
	char writeBuffer[65536];
	rapidjson::FileWriteStream os(fp, writeBuffer, sizeof(writeBuffer));
	rapidjson::Writer<rapidjson::FileWriteStream> writer(os);
	keystore.Accept(writer);
	fclose(fp);

	return random_bytes.substr(0, keybytes);
}
