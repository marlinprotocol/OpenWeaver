#include "Relay.hpp"
#include "NameGenerator.hpp"
#include <marlin/bsc/Abci.hpp>

#include <structopt/app.hpp>
#include <cryptopp/scrypt.h>
#include <cryptopp/modes.h>
#include <cryptopp/keccak.h>
#include <cryptopp/hex.h>

using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::beacon;
using namespace marlin::pubsub;


struct CliOptions {
	std::string discovery_addrs;
	std::string heartbeat_addrs;
	std::string datadir;
	std::string keystore_path;
	std::string keystore_pass_path;
	std::optional<uint16_t> pubsub_port;
	std::optional<uint16_t> discovery_port;
	std::optional<std::string> address;
	std::optional<std::string> name;
	std::optional<std::string> interface;
};
STRUCTOPT(CliOptions, discovery_addrs, heartbeat_addrs, datadir, keystore_path, keystore_pass_path, pubsub_port, discovery_port, address, name, interface);

std::string get_key(std::string keystore_path, std::string keystore_pass_path);

int main(int argc, char** argv) {
	try {
		auto options = structopt::app("bsc_relay").parse<CliOptions>(argc, argv);
		auto datadir = options.datadir;
		auto pubsub_port = options.pubsub_port.value_or(5000);
		auto discovery_port = options.discovery_port.value_or(5002);
		auto address = options.address.value_or("0x0000000000000000000000000000000000000000");

		std::string key = get_key(options.keystore_path, options.keystore_pass_path);
		if (key.empty()) {
			SPDLOG_ERROR("keystore file error");
			return 1;
		}
		if(address.size() != 42) {
			structopt::details::visitor visitor("bsc_relay", "");
			CliOptions opt = CliOptions();
			visit_struct::for_each(opt, visitor);
			visitor.optional_field_names.push_back("help");
			visitor.optional_field_names.push_back("version");
			throw structopt::exception(std::string("Invalid address"), visitor);
		}
		auto name = options.name.value_or(NameGenerator::generate());

		size_t pos;
		std::vector<SocketAddress> discovery_addrs;
		while((pos = options.discovery_addrs.find(',')) != std::string::npos) {
			discovery_addrs.push_back(SocketAddress::from_string(options.discovery_addrs.substr(0, pos)));
			options.discovery_addrs = options.discovery_addrs.substr(pos+1);
		}
		discovery_addrs.push_back(SocketAddress::from_string(options.discovery_addrs));

		std::vector<SocketAddress> heartbeat_addrs;
		while((pos = options.heartbeat_addrs.find(',')) != std::string::npos) {
			heartbeat_addrs.push_back(SocketAddress::from_string(options.heartbeat_addrs.substr(0, pos)));
			options.heartbeat_addrs = options.heartbeat_addrs.substr(pos+1);
		}
		heartbeat_addrs.push_back(SocketAddress::from_string(options.heartbeat_addrs));

		SPDLOG_INFO(
			"Starting relay named: {} on pubsub port: {}, discovery_port: {}",
			name,
			pubsub_port,
			discovery_port
		);

		Relay<true, true, true, marlin::bsc::Abci> relay(
			MASTER_PUBSUB_PROTOCOL_NUMBER,
			pubsub_port,
			SocketAddress::from_string(options.interface.value_or(std::string("0.0.0.0")).append(":").append(std::to_string(pubsub_port))),
			SocketAddress::from_string(options.interface.value_or(std::string("0.0.0.0")).append(":").append(std::to_string(discovery_port))),
			key,
			std::move(discovery_addrs),
			std::move(heartbeat_addrs),
			address,
			name,
			datadir
		);

		return EventLoop::run();
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

