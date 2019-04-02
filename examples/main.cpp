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

	uint64_t num_bytes = 0;

	void did_receive_bytes(Packet &&p, uint16_t stream_id, const SocketAddress &) {
		num_bytes += p.size();
		spdlog::info("Message received from stream {}: {} bytes", stream_id, p.size());
		spdlog::info("Total: {} bytes", num_bytes);
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

	#define SIZE 12500

	std::unique_ptr<char[]> data(new char[SIZE]);
	fill(data.get(), data.get()+SIZE, 'B');

	spdlog::info("Start");

	StreamProtocol<NodeType>::send_data(*b, std::move(data), SIZE, addr2);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
