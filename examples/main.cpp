#include <spdlog/spdlog.h>
#include <uv.h>
#include <cstring>
#include <algorithm>
#include <marlin/net/Node.hpp>
#include "protocol/StreamProtocol.hpp"


using namespace marlin::net;
using namespace marlin::stream;
using namespace std;


class TestNode: public Node<TestNode, StreamProtocol> {
public:
	StreamStorage<TestNode> stream_storage;

	TestNode(SocketAddress &_addr): Node(_addr) {
		StreamProtocol<TestNode>::setup(*this);
	}

	void did_receive_message(unique_ptr<char[]>, size_t size) {
		spdlog::info("Message Received: {} bytes", size);

		// for (int i=0; i<size; i++) {
		// 	if(message.get()[i] != 'B') {
		// 		spdlog::error("Message Error");
		// 		return;
		// 	}
		// }

		// spdlog::info("Message Verified");
	}
};


using NodeType = TestNode;

int main() {
	spdlog::default_logger()->set_level(spdlog::level::info);

	auto addr = SocketAddress::from_string("127.0.0.1:8000");
	auto b = new NodeType(addr);
	b->start_listening();

	auto addr2 = SocketAddress::from_string("127.0.0.1:8001");
	auto b2 = new NodeType(addr2);
	b2->start_listening();

	#define SIZE 12500000

	std::unique_ptr<char[]> data(new char[SIZE]);
	fill(data.get(), data.get()+SIZE, 'B');

	spdlog::info("Start");

	StreamProtocol<NodeType>::send_data(*b, std::move(data), SIZE, addr2);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
