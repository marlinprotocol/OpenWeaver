#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/tcp/TcpOutFiber.hpp>
#include <marlin/core/fibers/DynamicFramingFiber.hpp>
#include <marlin/core/fibers/SentinelFramingFiber.hpp>
#include <marlin/core/fibers/SentinelBufferFiber.hpp>
#include <marlin/core/fibers/LengthFramingFiber.hpp>
#include <marlin/core/fibers/LengthBufferFiber.hpp>
#include <uv.h>
#include <spdlog/spdlog.h>

#include <string>


using namespace marlin::core;
using namespace marlin::asyncio;


template<typename X>
using SentinelFramingFiberHelper = SentinelFramingFiber<X, '\n'>;

struct Grapher {
	using FiberType = Fabric<
		Grapher&,
		TcpOutFiber,
		DynamicFramingFiberHelper<
			FabricF<SentinelFramingFiberHelper, SentinelBufferFiber>::type,
			FabricF<LengthFramingFiber, LengthBufferFiber>::type
		>::type
	>;

	Timer t;
	SocketAddress dst;

	void query_cb() {
		SPDLOG_DEBUG("Timer hit: {}", dst.to_string());
		auto& fiber = *new FiberType(std::forward_as_tuple(
			*this,
			std::make_tuple(),
			std::make_tuple(std::make_tuple(
				std::make_tuple(),
				std::make_tuple()
			))
		));
		(void)fiber.template outer_call<"dial"_tag>(*this, dst);
	}

	template<typename... Args>
	Grapher(Args&&...) : t(this) {
		t.template start<Grapher, &Grapher::query_cb>(0, 5000);
	}

	size_t state = 0;
	size_t length = 0;

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "did_recv"_tag) {
			return did_recv(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_dial"_tag) {
			return did_dial(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_send"_tag) {
			return did_send(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_close"_tag) {
			return did_close(std::forward<decltype(args)>(args)...);
		} else {
			// static_assert(false) always breaks compilation
			// making it depend on a template parameter fixes it
			static_assert(tag < 0);
		}
	}

private:
	int did_recv(auto&& fiber, Buffer&& buf, SocketAddress addr [[maybe_unused]]) {
		SPDLOG_DEBUG("Grapher: Did recv: {} bytes from {}: {}", buf.size(), addr.to_string(), std::string((char*)buf.data(), buf.size()));

		if(state == 0) {
			// headers
			if(buf.size() == 2) {
				// empty
				if(length == 0) {
					// still do not have length
					fiber.template inner_call<"close"_tag>(*this);
					return -1;
				}

				// go to next phase
				state = 1;
				fiber.template inner_call<"transform"_tag>(*this, std::integral_constant<size_t, 1>{}, std::make_tuple(), std::make_tuple());
				fiber.template inner_call<"reset"_tag>(*this, length);
				return 0;
			} else if(std::memcmp(buf.data(), u8"content-length: ", 16) == 0) {
				// parse length
				auto length_str = std::string((char*)buf.data()+16, buf.size() - 18);
				length = std::stol(length_str);
			}
			fiber.template inner_call<"reset"_tag>(*this, 1000);
		} else {
			// body
			state = 0;
			length = 0;
			SPDLOG_INFO("{}", std::string((char*)buf.data(), buf.size()));
			fiber.template inner_call<"close"_tag>(*this);
		}

		return 0;
	}

	template<typename FiberType>
	int did_dial(FiberType& fiber, SocketAddress addr [[maybe_unused]]) {
		SPDLOG_DEBUG("Grapher: Did dial: {}", addr.to_string());
		fiber.template inner_call<"reset"_tag>(*this, 1000);
		auto query = Buffer(301).write_unsafe(
			0,
			(uint8_t*)"POST /subgraphs/name/marlinprotocol/staking-arb1 HTTP/1.0\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: 187\r\n"
			"\r\n"
			"{\"query\": \"query { clusters(first: 1000, where: {networkId: \\\"0xaaaebeba3810b1e6b70781f14b2d72c1cb89c0b2b320c43bb67ff79f562f5ff4\\\"}){clientKey, totalDelegations{token{tokenId} amount}}}\"}",
			301
		);
		fiber.template inner_call<"send"_tag>(*this, std::move(query));
		return 0;
	}

	template<typename FiberType>
	int did_send(FiberType&, Buffer&& buf [[maybe_unused]]) {
		SPDLOG_DEBUG("Grapher: Did send: {} bytes", buf.size());
		return 0;
	}

	int did_close(auto& fiber) {
		SPDLOG_DEBUG("Did close");
		delete &fiber;
		return 0;
	}
};


int main() {
	uv_getaddrinfo_t req;
	auto res = uv_getaddrinfo(uv_default_loop(), &req, [](uv_getaddrinfo_t*, int, addrinfo* res) {
		if(res != nullptr) {
			auto addr = SocketAddress(*res->ai_addr);

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
