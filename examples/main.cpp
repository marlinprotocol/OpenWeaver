#include <spdlog/spdlog.h>
#include <marlin/weave/node/Beacon.hpp>
#include <uv.h>
#include <cstring>
#include <algorithm>
#include "../include/protocol/StreamProtocol.hpp"


using namespace fiber;
using namespace weave;
using namespace stream;
using namespace std;


class TestNode: public Beacon {
public:
	StreamStorage<TestNode> stream_storage;
	TestNode(SocketAddress &_addr): Beacon(_addr) {
		this->register_protocol_cb(
			4,
			[this](fiber::Packet &&p, const SocketAddress &addr) {
				StreamProtocol<TestNode>::did_receive_packet(*this, std::move(p), addr);
			},
			[this](fiber::Packet &&p, const SocketAddress &addr) {
				StreamProtocol<TestNode>::did_send_packet(*this, std::move(p), addr);
			}
		);
	}

	void did_receive_message(unique_ptr<char[]> message, size_t size) {
		spdlog::debug("Message Received");
	}
};


using NodeType = TestNode;

int main() {
	spdlog::default_logger()->set_level(spdlog::level::debug);

	auto addr = SocketAddress::from_string("127.0.0.1:8000");
	auto b = new NodeType(addr);
	b->start_listening();

	auto addr2 = SocketAddress::from_string("127.0.0.1:8001");
	auto b2 = new NodeType(addr2);
	b2->start_listening();

	#define SIZE 2000

	std::unique_ptr<char[]> data(new char[SIZE]);
	fill(data.get(), data.get()+SIZE, 'A');

	StreamProtocol<NodeType>::send_data(*b, std::move(data), SIZE, addr2);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
