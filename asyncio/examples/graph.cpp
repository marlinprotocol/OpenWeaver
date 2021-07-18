#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/tcp/TcpOutFiber.hpp>
#include <marlin/core/fibers/FramingBaseFiber.hpp>
#include <marlin/core/fibers/SentinelFramingFiber.hpp>
#include <marlin/core/fibers/SentinelBufferFiber.hpp>
#include <uv.h>
#include <spdlog/spdlog.h>


using namespace marlin::core;
using namespace marlin::asyncio;


template<typename X>
using SentinelFramingFiberHelper = SentinelFramingFiber<X, '\n'>;

struct Grapher {
	using FiberType = Fabric<
		Grapher&,
		TcpOutFiber,
		FramingBaseFiber,
		SentinelFramingFiberHelper,
		SentinelBufferFiber
	>;

	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = false;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	Timer t;
	SocketAddress dst;

	void query_cb() {
		SPDLOG_INFO("Timer hit: {}", dst.to_string());
		auto& fiber = *new FiberType(std::forward_as_tuple(
			*this,
			std::make_tuple(),
			std::make_tuple(),
			std::make_tuple(),
			std::make_tuple()
		));
		(void)fiber.i(*this).dial(dst);
	}

	template<typename... Args>
	Grapher(Args&&...) : t(this) {
		t.template start<Grapher, &Grapher::query_cb>(0, 5000);
	}

	int did_recv(auto&& fiber, Buffer&& buf, SocketAddress addr) {
		SPDLOG_INFO("Grapher: Did recv: {} bytes from {}: {}", buf.size(), addr.to_string(), std::string((char*)buf.data(), buf.size()));
		fiber.o(*this).reset(1000000);
		return 0;
	}

	template<typename FiberType>
	int did_dial(FiberType& fiber, SocketAddress addr) {
		SPDLOG_INFO("Grapher: Did dial: {}", addr.to_string());
		fiber.o(*this).reset(1000000);
		auto query = Buffer(283).write_unsafe(
			0,
			(uint8_t*)"POST /subgraphs/name/marlinprotocol/staking HTTP/1.0\r\nContent-Type: application/json\r\nContent-Length: 174\r\n\r\n{\"query\": \"query { clusters(where: {networkId: \\\"0x9bd00430e53a5999c7c603cfc04cbdaf68bdbc180f300e4a2067937f57a0534f\\\"}){clientKey, totalDelegations{token{tokenId} amount}}}\"}",
			283
		);
		fiber.o(*this).send(*this, std::move(query));
		return 0;
	}

	template<typename FiberType>
	int did_send(FiberType&, Buffer&& buf) {
		SPDLOG_INFO("Grapher: Did send: {} bytes", buf.size());
		return 0;
	}

	int did_close(auto& fiber) {
		delete &fiber;
		return 0;
	}
};


int main() {
    uv_getaddrinfo_t req;
    auto res = uv_getaddrinfo(uv_default_loop(), &req, [](uv_getaddrinfo_t*, int, addrinfo* res) {
		if(res != nullptr) {
			auto& addr = *reinterpret_cast<SocketAddress*>(res->ai_addr);

			SPDLOG_INFO("DNS result: {}", addr.to_string());

			auto& g = *new Grapher();
			g.dst = addr;
			// g.dst = SocketAddress::loopback_ipv4(18000);
		}

        uv_freeaddrinfo(res);
    }, "graph.marlin.pro", "http", nullptr);
    if(res != 0) {
        SPDLOG_ERROR("DNS lookup error: {}", res);
        return -1;
    }

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
