#include <iostream>
#include <marlin/asyncio/tcp/TcpOutFiber.hpp>
#include <marlin/core/fibers/DynamicFramingFiber.hpp>
#include <marlin/core/fibers/SentinelFramingFiber.hpp>
#include <marlin/core/fibers/SentinelBufferFiber.hpp>
#include <marlin/core/fibers/LengthFramingFiber.hpp>
#include <marlin/core/fibers/LengthBufferFiber.hpp>
#include <uv.h>
#include <marlin/beacon/DiscoveryServer.hpp>
#include <marlin/beacon/ClusterDiscoverer.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>
#include <marlin/asyncio/udp/UdpFiber.hpp>
#include <marlin/core/fibers/VersioningFiber.hpp>
#include <marlin/core/fabric/Fabric.hpp>
#include <cstring>
#include <fstream>
#include <string>
#include <boost/filesystem.hpp>
#include <map>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/fmt/fmt.h>
#include <rapidjson/document.h>
#include <structopt/app.hpp>


using namespace marlin;
using namespace marlin::core;
using namespace marlin::asyncio;


std::map< std::string, std::string> MData;

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
		(void)fiber.i(*this).dial(dst);
	}

	template<typename... Args>
	Grapher(Args&&...) {
		//t.template start<Grapher, &Grapher::query_cb>(0, 5000);
	}

	size_t state = 0;
	size_t length = 0;

	int did_recv(auto&& fiber, Buffer&& buf, SocketAddress addr [[maybe_unused]]) {
		SPDLOG_DEBUG("Grapher: Did recv: {} bytes from {}: {}", buf.size(), addr.to_string(), std::string((char*)buf.data(), buf.size()));

		if(state == 0) {
			// headers
			if(buf.size() == 2) {
				// empty
				if(length == 0) {
					// still do not have length
					fiber.o(*this).close();
					return -1;
				}

				// go to next phase
				state = 1;
				fiber.o(*this).template transform<1>(std::make_tuple(), std::make_tuple());
				fiber.o(*this).reset(length);
				return 0;
			} else if(std::memcmp(buf.data(), u8"content-length: ", 16) == 0) {
				// parse length
				auto length_str = std::string((char*)buf.data()+16, buf.size() - 18);
				length = std::stol(length_str);
			}
			fiber.o(*this).reset(1000);
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
					MData[cluster["clientKey"].GetString()] = cluster["id"].GetString();
				}
			}else{
				SPDLOG_INFO("Not Parsed");
			}
			fiber.o(*this).close();
		}
		return 0;
	}

	template<typename FiberType>
	int did_dial(FiberType& fiber, SocketAddress addr [[maybe_unused]]) {
		SPDLOG_DEBUG("Grapher: Did dial: {}", addr.to_string());
		fiber.o(*this).reset(1000);
		auto query = Buffer(259).write_unsafe(
			0,
			(uint8_t*)"POST /subgraphs/name/marlinprotocol/staking HTTP/1.0\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: 150\r\n"
			"\r\n"
			"{\"query\": \"query { clusters(first: 1000, where: {networkId: \\\"0xaaaebeba3810b1e6b70781f14b2d72c1cb89c0b2b320c43bb67ff79f562f5ff4\\\"}){clientKey, id}}\"}",
			259
		);
		fiber.o(*this).send(*this, std::move(query));
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


class BeaconDelegate {
public:
	std::vector<std::tuple<uint32_t, uint16_t, uint16_t>> get_protocols() {
		return {};
	}

	void new_peer_protocol(
		std::array<uint8_t, 20> client_key,
		core::SocketAddress const& addr,
		uint8_t const* static_pk,
		uint32_t protocol,
		uint16_t version
	) {
		if(client_key == std::array<uint8_t, 20>({})) {
			// Skip zero
			return;
		}
		auto hexstring = fmt::format("0x{:spn}", spdlog::to_hex(client_key.data(), client_key.data()+20));
		auto found = MData.find(hexstring);
		if(found == MData.end()){
			SPDLOG_INFO(
			"not found: {}", hexstring
			);
		}else{
			SPDLOG_INFO(
				"New peer: {}, {}, {:spn}, {}, {}",
				found->second,
				addr.to_string(),
				spdlog::to_hex(static_pk, static_pk+32),
				protocol,
				version
			);
		}
	}

	void new_cluster(
		core::SocketAddress const& addr,
		std::array<uint8_t, 20> const& client_key
	) {
		if(client_key == std::array<uint8_t, 20>({})) {
			// Skip zero
			return;
		}
		auto hexstring = fmt::format("0x{:spn}", spdlog::to_hex(client_key.data(), client_key.data()+20));
		auto found = MData.find(hexstring);
		if(found == MData.end()){
			SPDLOG_INFO(
			"not found: {}", hexstring
			);
		}else{
			SPDLOG_INFO(
				"New cluster: {}, {}",
				found->second,
				addr.to_string()
			);
		}
	}
};

struct CliOptions {
	std::optional<std::string> discovery_addr;
	std::optional<std::string> bootstrap_addr;
};
STRUCTOPT(CliOptions, discovery_addr, bootstrap_addr);

int query() {
    uv_getaddrinfo_t req;
    auto res = uv_getaddrinfo(uv_default_loop(), &req, [](uv_getaddrinfo_t*, int, addrinfo* res) {
		if(res != nullptr) {
			auto& addr = *reinterpret_cast<SocketAddress*>(res->ai_addr);

			SPDLOG_INFO("DNS result: {}", addr.to_string());

			auto& g = *new Grapher();
			g.dst = addr;
			g.query_cb();
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

int main(int argc, char** argv) {
	query();
	try {
		auto options = structopt::app("bridge").parse<CliOptions>(argc, argv);
		auto baddr = core::SocketAddress::from_string(
			options.bootstrap_addr.value_or("34.82.79.68:8002")
		);
		auto caddr = core::SocketAddress::from_string(
			options.discovery_addr.value_or("0.0.0.0:18002")
		);

		uint8_t static_sk[crypto_box_SECRETKEYBYTES];
		uint8_t static_pk[crypto_box_PUBLICKEYBYTES];
		crypto_box_keypair(static_pk, static_sk);

		BeaconDelegate del;

		auto crawler = new beacon::ClusterDiscoverer<
			BeaconDelegate
		>(caddr, static_sk);
		crawler->delegate = &del;

		crawler->start_discovery(baddr);

		return asyncio::EventLoop::run();
	} catch (structopt::exception& e) {
		SPDLOG_ERROR("{}", e.what());
		SPDLOG_ERROR("{}", e.help());
	}

	return -1;
}
