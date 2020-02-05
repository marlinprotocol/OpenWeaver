/*
	- tcp server's enable cut through : false
	- blockchainChannel : tendermint (can be configured according to the project. However not required to do so)
*/

#include <sodium.h>

#include <marlin/multicast/DefaultMulticastClient.hpp>
#include <marlin/net/tcp/TcpTransportFactory.hpp>
#include <marlin/stream/StreamTransportFactory.hpp>
#include <marlin/lpf/LpfTransportFactory.hpp>

using namespace marlin::multicast;
using namespace marlin::net;
using namespace marlin::stream;
using namespace marlin::lpf;

class MulticastDelegate;

const bool enable_cut_through = false;
std::string blockchainChannel = "tendermint";

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
	DefaultMulticastClient<MulticastDelegate>* multicastClient;
	LpfTcpTransport* tcpClient = nullptr;
	LpfTcpTransportFactory f;

	MulticastDelegate(DefaultMulticastClientOptions clop, std::string lpftcp_server_addr) {
		multicastClient = new DefaultMulticastClient<MulticastDelegate> (clop);
		multicastClient->delegate = this;

		// bind to address and start listening on tcpserver
		f.bind(SocketAddress::from_string(lpftcp_server_addr));
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
	int did_recv_message(LpfTcpTransport &transport, Buffer &&message) {
		SPDLOG_DEBUG(
			"Did recv from blockchain client, message with length {}",
			message.size()
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

	void did_send_message(LpfTcpTransport &transport, Buffer &&message) {}

	// TODO:
	void did_close(LpfTcpTransport &transport) {
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

	void did_recv_message(
		DefaultMulticastClient<MulticastDelegate> &client,
		Buffer &&message,
		Buffer &&witness,
		std::string &channel,
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
		DefaultMulticastClient<MulticastDelegate> &client,
		std::string &channel
	) {}

	void did_unsubscribe(
		DefaultMulticastClient<MulticastDelegate> &client,
		std::string &channel
	) {}
};

int main(int , char **argv) {

	std::string beacon_addr = std::string(argv[1]);
	std::string discovery_addr, pubsub_addr, lpftcp_server_addr;

	discovery_addr = "0.0.0.0:15002";
	pubsub_addr = "0.0.0.0:15000";
	lpftcp_server_addr = "0.0.0.0:15003";

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);

	DefaultMulticastClientOptions clop {
		static_sk,
		std::vector<std::string>(1, blockchainChannel),
		beacon_addr,
		discovery_addr,
		pubsub_addr
	};

	MulticastDelegate del(clop, lpftcp_server_addr);

	return DefaultMulticastClient<MulticastDelegate>::run_event_loop();
}
