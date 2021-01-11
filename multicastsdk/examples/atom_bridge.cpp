/*
	- tcp server's enable cut through : false
	- blockchainChannel : tendermint (can be configured according to the project. However not required to do so)
*/

#include <sodium.h>
#include <unistd.h>

#include <marlin/multicast/DefaultMulticastClient.hpp>
#include <marlin/asyncio/tcp/TcpTransportFactory.hpp>
#include <marlin/stream/StreamTransportFactory.hpp>
#include <marlin/lpf/LpfTransportFactory.hpp>
#include <marlin/pubsub/attestation/SigAttester.hpp>


using namespace marlin::multicast;
using namespace marlin::pubsub;
using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::stream;
using namespace marlin::lpf;

class MulticastDelegate;

const bool enable_cut_through = false;
uint16_t blockchainChannel = 0;

using LpfTcpTransportFactory = LpfTransportFactory<
	MulticastDelegate,
	MulticastDelegate,
	TcpTransportFactory,
	TcpTransport,
	enable_cut_through
>;
using LpfTcpTransport = LpfTransport<
	MulticastDelegate,
	TcpTransport,
	enable_cut_through
>;

class MulticastDelegate {
public:
	DefaultMulticastClient<MulticastDelegate, SigAttester>* multicastClient;
	LpfTcpTransport* tcpClient = nullptr;
	LpfTcpTransportFactory f;

	MulticastDelegate(DefaultMulticastClientOptions clop, std::string lpftcp_bridge_addr) {
		multicastClient = new DefaultMulticastClient<MulticastDelegate, SigAttester> (clop);
		multicastClient->delegate = this;

		// bind to address and start listening on tcpserver
		f.bind(SocketAddress::from_string(lpftcp_bridge_addr));
		f.listen(*this);
	}

	//-----------------------delegates for Lpf-Tcp-Transport-----------------------------------

	// Listen delegate
	bool should_accept(SocketAddress const &addr) {
		return true;
	}

	void did_create_transport(LpfTcpTransport &transport) {
		SPDLOG_DEBUG(
			"DID CREATE LPF TRANSPORT: {}",
			transport.dst_addr.to_string()
		);

		transport.setup(this, NULL);

		tcpClient = &transport;
	}

	// Transport delegate

	// not required because server
	void did_dial(LpfTcpTransport &transport) {
		SPDLOG_DEBUG(
			"DID DIAL: {}",
			transport.dst_addr.to_string()
		);
	}

	// forward on marlin multicast
	int did_recv(LpfTcpTransport &transport, Buffer &&message) {
		SPDLOG_INFO(
			"Did recv from blockchain client, message with length {}: {}",
			message.size(),
			spdlog::to_hex(message.data(), message.data() + message.size())
		);

		SPDLOG_DEBUG(
			"Sending to marlin multicast, message with length {}",
			message.size()
		);

		multicastClient->ps.send_message_on_channel(
			blockchainChannel,
			message.data(),
			message.size()
		);

		return 0;
	}

	void did_send(LpfTcpTransport &transport, Buffer &&message) {}

	// TODO:
	void did_close(LpfTcpTransport &transport, uint16_t) {
		SPDLOG_DEBUG(
			"Closed connection with client: {}",
			transport.dst_addr.to_string()
		);
		tcpClient = nullptr;
	}

	// TODO: not required because server
	int dial(SocketAddress const &addr, uint8_t const *remote_static_pk) {
		return 0;
	};

	//-----------------------delegates for DefaultMultiCastClient-------------------------------

	template<typename T> // TODO: Code smell, remove later
	void did_recv(
		DefaultMulticastClient<MulticastDelegate, SigAttester> &client,
		Buffer &&message,
		T header,
		uint16_t channel,
		uint64_t message_id
	) {
		SPDLOG_DEBUG(
			"Did recv from multicast, message-id: {}",
			message_id
		);

		//TODO: send on tcp client
		if (channel==blockchainChannel && tcpClient != nullptr) {
			SPDLOG_DEBUG(
				"Sending to blockchain client"
			);
			tcpClient->send(std::move(message));
		}
	}

	void did_subscribe(
		DefaultMulticastClient<MulticastDelegate, SigAttester> &client,
		uint16_t channel
	) {}

	void did_unsubscribe(
		DefaultMulticastClient<MulticastDelegate, SigAttester> &client,
		uint16_t channel
	) {}
};

int main(int argc, char **argv) {

	std::string beacon_addr = "127.0.0.1:8002";
	std::string discovery_addr = "0.0.0.0:22002";
	std::string pubsub_addr = "0.0.0.0:22000";
	std::string lpftcp_bridge_addr = "0.0.0.0:22401";

	char c;
	while ((c = getopt (argc, argv, "b::d::p::l::")) != -1) {
		switch (c) {
			case 'b':
				beacon_addr = std::string(optarg);
				break;
			case 'd':
				discovery_addr = std::string(optarg);
				break;
			case 'p':
				pubsub_addr = std::string(optarg);
				break;
			case 'l':
				lpftcp_bridge_addr = std::string(optarg);
				break;
			default:
			return 1;
		}
	}

	SPDLOG_INFO(
		"Beacon: {}, Discovery: {}, PubSub: {} TcpBridge: {}",
		beacon_addr,
		discovery_addr,
		pubsub_addr,
		lpftcp_bridge_addr
	);

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);

	DefaultMulticastClientOptions clop {
		static_sk,
		static_pk,
		std::vector<uint16_t>(1, blockchainChannel),
		beacon_addr,
		discovery_addr,
		pubsub_addr
	};

	MulticastDelegate del(clop, lpftcp_bridge_addr);

	return DefaultMulticastClient<MulticastDelegate, SigAttester>::run_event_loop();
}
