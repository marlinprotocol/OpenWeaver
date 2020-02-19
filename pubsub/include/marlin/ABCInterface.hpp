/*
	1. loop to retry connection to contract interface on restart broken connection
	2. maps to store balance
*/
#ifndef MARLIN_ABCInterface_HPP
#define MARLIN_ABCInterface_HPP

#include <fstream>
#include <experimental/filesystem>
#include <sodium.h>
#include <unistd.h>

#include <marlin/net/tcp/TcpTransportFactory.hpp>
#include <marlin/stream/StreamTransportFactory.hpp>
#include <marlin/lpf/LpfTransportFactory.hpp>


using namespace marlin;
using namespace marlin::net;
using namespace marlin::stream;
using namespace marlin::lpf;

const bool enable_cut_through = false;

class ABCInterface;

using LpfTcpTransportFactory = LpfTransportFactory<
	ABCInterface,
	ABCInterface,
	TcpTransportFactory,
	TcpTransport,
	enable_cut_through
>;
using LpfTcpTransport = LpfTransport<
	ABCInterface,
	TcpTransport,
	enable_cut_through
>;

class ABCInterface {
public:
	LpfTcpTransportFactory f;
	LpfTcpTransport* contractInterface = nullptr;


	ABCInterface() {
		// dial(net::SocketAddress::from_string("0.0.0.0:5000"), NULL);
	}

	//-----------------------delegates for Lpf-Tcp-Transport-----------------------------------

	// Listen delegate: Not required
	bool should_accept(SocketAddress const &addr __attribute__((unused))) {
		return true;
	}

	void did_create_transport(LpfTcpTransport &transport) {
		SPDLOG_DEBUG(
			"DID CREATE LPF TRANSPORT: {}",
			transport.dst_addr.to_string()
		);

		transport.setup(this, NULL);

		contractInterface = &transport;
	}

	// Transport delegate

	// TODO?
	void did_dial(LpfTcpTransport &transport) {
		SPDLOG_DEBUG(
			"DID DIAL: {}",
			transport.dst_addr.to_string()
		);
	}

	// forward on marlin multicast
	int did_recv_message(
		LpfTcpTransport &transport __attribute__((unused)),
		Buffer &&message
	) {
		SPDLOG_DEBUG(
			"Did recv from blockchain client, message with length {}",
			message.size()
		);

		return 0;
	}

	void did_send_message(
		LpfTcpTransport &transport __attribute__((unused)),
		Buffer &&message __attribute__((unused))
	) {}

	// TODO:
	void did_close(LpfTcpTransport &transport) {
		SPDLOG_DEBUG(
			"Closed connection with client: {}",
			transport.dst_addr.to_string()
		);

		contractInterface = nullptr;
	}

	int dial(SocketAddress const &addr, uint8_t const *remote_static_pk) {
		SPDLOG_DEBUG(
			"SENDING DIAL TO: {}",
			addr.to_string()
		);

		return f.dial(addr, *this, remote_static_pk);
	};


	void send_message(
		std::string channel __attribute__((unused)),
		uint64_t message_id __attribute__((unused)),
		const char *data __attribute__((unused))
	) {}

	// Enquiry calls by
	//checkBalance(xyz)

};

#endif // MARLIN_ABCInterface_HPP
