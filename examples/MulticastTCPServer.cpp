/*
1. value of enable cut through
2. did_close
3. did_recv
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

	// delegates for LPFTCPTransport

	// Listen delegate
	bool should_accept(SocketAddress const &addr) {
		return true;
	}

	void did_create_transport(LpfTcpTransport &transport) {
		SPDLOG_DEBUG(
			"DID CREATE TRANSPORT: {}",
			transport.dst_addr.to_string()
		);

		transport.setup(this, NULL);
	}

	// Transport delegate

	// TODO: not required because server
	void did_dial(LpfTcpTransport &transport) {
		SPDLOG_DEBUG(
			"DID DIAL: {}",
			transport.dst_addr.to_string()
		);
	}

	// TODO: forward on marlin multicast
	int did_recv_message(LpfTcpTransport &transport, Buffer &&message) {
		SPDLOG_DEBUG(
			"DID RECV message"
		);
	}

	void did_send_message(LpfTcpTransport &transport, Buffer &&message) {}

	// TODO:
	void did_close(LpfTcpTransport &transport);

	// TODO: not required because server
	int dial(SocketAddress const &addr, uint8_t const *remote_static_pk) {
	};


	// delegates for DefaultMultiCastClient
	void did_recv_message(
		DefaultMulticastClient<MulticastDelegate> &client,
		Buffer &&message,
		Buffer &&witness,
		std::string &channel,
		uint64_t message_id
	) {
		SPDLOG_INFO(
			"Did recv message: {}",
			message_id
		);
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

	MulticastDelegate del;

	std::string beacon_addr = std::string(argv[1]);
	std::string discovery_addr = "0.0.0.0:15002";
	std::string pubsub_addr = "0.0.0.0:15000";
	std::string lpftcp_server_addr = "0.0.0.8000";

	uint8_t static_sk[crypto_box_SECRETKEYBYTES];
	uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
	crypto_box_keypair(static_pk, static_sk);

	DefaultMulticastClientOptions clop {
		static_sk,
		{"eth"},
		beacon_addr,
		discovery_addr,
		pubsub_addr
	};

	DefaultMulticastClient<MulticastDelegate> cl(clop);
	cl.delegate = &del;

	// create LPFTransportFactory
	LpfTcpTransportFactory f;
	// bind to address and start the LPFTransport
	f.bind(SocketAddress::from_string(lpftcp_server_addr));

	// setup delegate and listen for LPFTransport
	f.listen(del);

	return DefaultMulticastClient<MulticastDelegate>::run_event_loop();
}
