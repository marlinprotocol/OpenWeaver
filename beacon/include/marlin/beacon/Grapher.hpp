#include <marlin/asyncio/core/Timer.hpp>
#include <marlin/asyncio/tcp/TcpOutFiber.hpp>
#include <marlin/core/fibers/DynamicFramingFiber.hpp>
#include <marlin/core/fibers/SentinelFramingFiber.hpp>
#include <marlin/core/fibers/SentinelBufferFiber.hpp>
#include <marlin/core/fibers/LengthFramingFiber.hpp>
#include <marlin/core/fibers/LengthBufferFiber.hpp>
#include <marlin/core/fabric/Fabric.hpp>

#include <uv.h>
#include <spdlog/fmt/fmt.h>
#include <rapidjson/document.h>
#include <string>
#include <map>



namespace marlin{
namespace beacon{

template<typename X>
using SentinelFramingFiberHelper = core::SentinelFramingFiber<X, '\n'>;

struct Grapher {
	using FiberType = core::Fabric<
		Grapher&,
		asyncio::TcpOutFiber,
		core::DynamicFramingFiberHelper<
			core::FabricF<SentinelFramingFiberHelper, core::SentinelBufferFiber>::type,
			core::FabricF<core::LengthFramingFiber, core::LengthBufferFiber>::type
		>::type
	>;

	asyncio::Timer t;
	core::SocketAddress dst;
	std::map< std::string, std::string> clientkey_id_map;

	void query_subgraph() {
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
		//query();
		t.template start<Grapher, &Grapher::query>(0, 300000);
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
			static_assert(tag < 0);
		}
	}

private:
	int did_recv(auto&& fiber, core::Buffer&& buf, core::SocketAddress addr [[maybe_unused]]) {
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
			// SPDLOG_INFO("{}", std::string((char*)buf.data(), buf.size()));
			rapidjson::Document JData;
			JData.Parse((char*)buf.data(), buf.size());
			if(!JData.HasParseError()){
				auto& clusters = JData["data"]["clusters"];
				for(rapidjson::SizeType i = 0; i < clusters.Size(); i++){
					auto& cluster = clusters[i];
					clientkey_id_map[cluster["clientKey"].GetString()] = cluster["id"].GetString();
					// SPDLOG_INFO("clientKey {}:id {}", cluster["clientKey"].GetString(), cluster["id"].GetString());
				}
			}else{
				SPDLOG_INFO("Not Parsed");
			}
			fiber.template inner_call<"close"_tag>(*this);
		}
		return 0;
	}

	template<typename FiberType>
	int did_dial(FiberType& fiber, core::SocketAddress addr [[maybe_unused]]) {
		SPDLOG_DEBUG("Grapher: Did dial: {}", addr.to_string());
		fiber.template inner_call<"reset"_tag>(*this, 1000);
		auto query = core::Buffer(264).write_unsafe(
			0,
			(uint8_t*)"POST /subgraphs/name/marlinprotocol/staking-arb1 HTTP/1.0\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: 150\r\n"
			"\r\n"
			"{\"query\": \"query { clusters(first: 1000, where: {networkId: \\\"0xaaaebeba3810b1e6b70781f14b2d72c1cb89c0b2b320c43bb67ff79f562f5ff4\\\"}){clientKey, id}}\"}",
			264
		);
		fiber.template inner_call<"send"_tag>(*this, std::move(query));
		return 0;
	}

	template<typename FiberType>
	int did_send(FiberType&, core::Buffer&& buf [[maybe_unused]]) {
		SPDLOG_DEBUG("Grapher: Did send: {} bytes", buf.size());
		return 0;
	}

	int did_close(auto& fiber) {
		SPDLOG_DEBUG("Did close");
		delete &fiber;
		return 0;
	}

	void query() {
		uv_getaddrinfo_t* req = new uv_getaddrinfo_t();
		req->data = this;
		auto res = uv_getaddrinfo(uv_default_loop(), req, [](uv_getaddrinfo_t* handle, int, addrinfo* res) {
			if(res != nullptr) {
			auto& addr = *reinterpret_cast<core::SocketAddress*>(res->ai_addr);

			SPDLOG_INFO("DNS result: {}", addr.to_string());
					Grapher *grapher= (Grapher*)handle->data;
			grapher->dst = addr;
			grapher->query_subgraph();
			delete handle;
			// g.dst = SocketAddress::loopback_ipv4(18000);
			}

			uv_freeaddrinfo(res);
		}, "graph.marlin.pro", "http", nullptr);
		if(res != 0) {
			SPDLOG_ERROR("DNS lookup error: {}", res);
		}

		// uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	}

};

}
}
