#include "Socket.hpp"
#include <spdlog/spdlog.h>

namespace marlin {
namespace net {

Socket::Socket() {}

int Socket::bind(const SocketAddress &addr) {
	this->addr = addr;

	uv_loop_t *loop = uv_default_loop();

	int res = uv_udp_init(loop, &_socket);
	if (res < 0) {
		spdlog::error("Fiber: Socket: Init error: {}", res);
		return res;
	}

	res = uv_udp_bind(&_socket, reinterpret_cast<const sockaddr *>(&this->addr), 0);
	if (res < 0) {
		spdlog::error("Fiber: Socket: Bind error: {}", res);
		return res;
	}

	return 0;
}

} // namespace net
} // namespace marlin
