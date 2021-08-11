#include <marlin/pubsub/PubSubNode.hpp>
#include <marlin/beacon/DiscoveryServer.hpp>
#include <marlin/beacon/DiscoveryClient.hpp>
#include <unistd.h>
#include <marlin/pubsub/EmptyAbci.hpp>
#include <marlin/pubsub/attestation/LpfAttester.hpp>
#include <marlin/pubsub/witness/LpfBloomWitnesser.hpp>

#include <structopt/app.hpp>
#include <cryptopp/scrypt.h>
#include <cryptopp/modes.h>
#include <cryptopp/keccak.h>
#include <cryptopp/hex.h>
#include <cryptopp/aes.h>
#include <rapidjson/document.h>
#include <boost/filesystem.hpp>


using namespace marlin;
using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::beacon;
using namespace marlin::pubsub;

#define PUBSUB_PROTOCOL_NUMBER 0x10000000

class Goldfish {
public:
	using Self = Goldfish;
	using PubSubNodeType = PubSubNode<
		Self,
		true,
		true,
		true,
		LpfAttester,
		LpfBloomWitnesser,
		EmptyAbci
	>;

	uint16_t ps_port;

	Goldfish(uint16_t port = 10000) {
		ps_port = port;
	}

	DiscoveryClient<Goldfish> *b;
	PubSubNodeType *ps;

	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {
			std::make_tuple(PUBSUB_PROTOCOL_NUMBER, 0, ps_port)
		};
	}

	void new_peer_protocol(
		core::SocketAddress const &addr,
		uint8_t const* static_pk,
		uint32_t protocol,
		uint16_t
	) {
		if(protocol == PUBSUB_PROTOCOL_NUMBER) {
			SPDLOG_INFO("New peer: {}, {:spn}", addr.to_string(), spdlog::to_hex(static_pk, static_pk+32));
			ps->subscribe(addr, static_pk);
		}
	}

	std::vector<uint16_t> channels = {0};

	void did_unsubscribe(
		PubSubNodeType &,
		uint16_t channel [[maybe_unused]]
	) {
		SPDLOG_DEBUG("Did unsubscribe: {}", channel);
	}

	void did_subscribe(
		PubSubNodeType &,
		uint16_t channel [[maybe_unused]]
	) {
		SPDLOG_DEBUG("Did subscribe: {}", channel);
	}

	void did_recv(
		PubSubNodeType &,
		Buffer &&,
		typename PubSubNodeType::MessageHeaderType,
		uint16_t channel [[maybe_unused]],
		uint64_t message_id [[maybe_unused]]
	) {
		SPDLOG_INFO(
			"Received message {} on channel {}",
			message_id,
			channel
		);
	}

	bool should_accept(SocketAddress const &) {
		return true;
	}

	void manage_subscriptions(
		typename PubSubNodeType::ClientKey,
		size_t,
		typename PubSubNodeType::TransportSet&,
		typename PubSubNodeType::TransportSet&) {
	}
};

struct CliOptions {
	std::optional<std::string> discovery_addr;
	std::optional<std::string> pubsub_addr;
	std::optional<std::string> beacon_addr;
	std::optional<std::string> listen_addr;
	std::optional<std::string> keystore_path;
	std::optional<std::string> keystore_pass_path;
	enum class Contracts { mainnet, kovan };
	std::optional<Contracts> contracts;
};
STRUCTOPT(CliOptions, discovery_addr, pubsub_addr, beacon_addr, listen_addr, keystore_path, keystore_pass_path, contracts);

std::string get_key(std::string keystore_path, std::string keystore_pass_path);

int main(int argc, char **argv) {
	if(sodium_init() == -1) {
		return -1;
	}

	std::string beacon_addr("127.0.0.1:9002"),
				heartbeat_addr("127.0.0.1:9003");

	char c;
	while ((c = getopt (argc, argv, "b::d::p::")) != -1) {
		switch (c) {
			case 'b':
			beacon_addr = std::string(optarg);
			break;
			default:
			return 1;
		}
	}

	SPDLOG_INFO(
		"Beacon: {}",
		beacon_addr
	);

	Goldfish g(20000);

	std::string cbaddr("127.0.0.1:7002"), chaddr("127.0.0.1:7003");
	DiscoveryServer<Goldfish> cb(SocketAddress::from_string(cbaddr), SocketAddress::from_string(chaddr));
	cb.delegate = &g;

	// Beacon
	auto key = get_key("/home/roshan/Downloads/nitinkeystore", "/home/roshan/Downloads/nitinpass");
	DiscoveryServer<Goldfish> b(SocketAddress::from_string(beacon_addr), SocketAddress::from_string(heartbeat_addr), SocketAddress::from_string(chaddr), key);
	b.delegate = &g;

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);

	// Pubsub
	Goldfish::PubSubNodeType ps(SocketAddress::from_string("127.0.0.1:20000"), 1000, 1000, static_sk, std::forward_as_tuple("/subgraphs/name/marlinprotocol/staking", ""), std::make_tuple(), std::make_tuple(static_pk));
	ps.delegate = &g;
	g.ps = &ps;

	// Discovery client
	DiscoveryClient<Goldfish> dc(SocketAddress::from_string("127.0.0.1:10002"), static_sk);
	dc.delegate = &g;
	dc.is_discoverable = true;
	g.b = &dc;

	Goldfish g2(21000);

	uint8_t static_sk2[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk2[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk2, static_sk2);

	// Pubsub
	Goldfish::PubSubNodeType ps2(SocketAddress::from_string("127.0.0.1:21000"), 1000, 1000, static_sk2, std::forward_as_tuple("/subgraphs/name/marlinprotocol/staking", ""), std::make_tuple(), std::make_tuple(static_pk2));
	ps2.delegate = &g2;
	g2.ps = &ps2;

	// Discovery client
	DiscoveryClient<Goldfish> dc2(SocketAddress::from_string("127.0.0.1:11002"), static_sk2);
	dc2.delegate = &g2;
	dc2.is_discoverable = true;
	g2.b = &dc2;

	dc.start_discovery({SocketAddress::from_string(beacon_addr)},{SocketAddress::from_string(heartbeat_addr)});
	dc2.start_discovery({SocketAddress::from_string(beacon_addr)},{SocketAddress::from_string(heartbeat_addr)});

	return EventLoop::run();
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
