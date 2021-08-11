#include <marlin/multicast/DefaultMulticastClient.hpp>
#include <random>
#include <marlin/pubsub/attestation/SigAttester.hpp>

// #include <structopt/app.hpp>
#include <cryptopp/scrypt.h>
#include <cryptopp/modes.h>
#include <cryptopp/keccak.h>
#include <cryptopp/hex.h>
#include <cryptopp/aes.h>
#include <rapidjson/document.h>
#include <boost/filesystem.hpp>

using namespace marlin::multicast;
using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::pubsub;

class MulticastDelegate {
public:
	template<typename T> // TODO: Code smell, remove later
	void did_recv(
		DefaultMulticastClient<MulticastDelegate, SigAttester, LpfBloomWitnesser> &client,
		Buffer &&message,
		T header,
		uint16_t channel,
		uint64_t message_id
	) {
		if((message_id & 0x0) == 0) {
			SPDLOG_INFO(
				"Received message {} on channel {}",
				message_id,
				channel
			);
		}
	}

	void did_subscribe(
		DefaultMulticastClient<MulticastDelegate, SigAttester, LpfBloomWitnesser> &client,
		uint16_t channel
	) {}

	void did_unsubscribe(
		DefaultMulticastClient<MulticastDelegate, SigAttester, LpfBloomWitnesser> &client,
		uint16_t channel
	) {}
};

constexpr uint msg_rate = 1;
constexpr uint msg_size = 500000;
constexpr float randomness = 1;

std::mt19937 gen((std::random_device())());

uint8_t msg[msg_size];

void msggen_timer_cb(uv_timer_t *handle) {
	auto &client = *(DefaultMulticastClient<MulticastDelegate, SigAttester, LpfBloomWitnesser> *)handle->data;

	if(std::generate_canonical<float, 10>(gen) > randomness) {
		return;
	}

	for (uint i = 0; i < msg_rate; ++i) {
		auto message_id = client.ps.send_message_on_channel(
			0,
			msg,
			msg_size
		);
		if((message_id & 0x0) == 0) {
			SPDLOG_INFO(
				"Received message {} on channel {}",
				message_id,
				0
			);
		}
	}
}

std::string get_key(std::string keystore_path, std::string keystore_pass_path);

int main(int , char **argv) {
	for (uint i = 0; i < msg_size; ++i)
	{
		msg[i] = (uint8_t)i;
	}

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);

	MulticastDelegate del;

	auto key = get_key("/home/roshan/Downloads/nitinkeystore", "/home/roshan/Downloads/nitinpass");
	DefaultMulticastClientOptions clop {
		static_sk,
		static_pk,
		{0},
		std::string(argv[1]),
		"0.0.0.0:15002",
		"0.0.0.0:15000"
	};
	DefaultMulticastClient<MulticastDelegate, SigAttester, LpfBloomWitnesser> cl(clop, (uint8_t*)key.data());
	cl.delegate = &del;

	uv_timer_t msggen_timer;
	msggen_timer.data = &cl;
	uv_timer_init(uv_default_loop(), &msggen_timer);
	uv_timer_start(&msggen_timer, msggen_timer_cb, 1000, 1000);

	return DefaultMulticastClient<MulticastDelegate, SigAttester, LpfBloomWitnesser>::run_event_loop();
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
