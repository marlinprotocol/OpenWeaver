#ifndef MARLIN_ASYNCIO_UDP_DUMMYSTREAMTRANSPORT_HPP
#define MARLIN_ASYNCIO_UDP_DUMMYSTREAMTRANSPORT_HPP

#include <marlin/core/Buffer.hpp>
#include <spdlog/spdlog.h>
#include <marlin/core/SocketAddress.hpp>

using namespace marlin::core;

namespace marlin {
namespace stream {


template <typename TransportType>
class StreamFiber {
private:
	using Self = StreamFiber<TransportType>;
	SocketAddress addr;

public:
	using OuterMessageType = Buffer;
	using InnerMessageType = Buffer;
	StreamFiber() {}
	template <typename Args>
	StreamFiber(Args&&...) {}
	StreamFiber(SocketAddress addr) : addr(addr) {}

	int did_recv(auto&&, Buffer &&buf, SocketAddress addr) {
		SPDLOG_INFO(
			"Received in StreamFiber: {} , {}", 
			buf.size(),
			addr.get_port()
		);
		return 0;
	}
};

}
}


#endif