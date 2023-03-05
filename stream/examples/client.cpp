#include <marlin/core/fabric/Fabric.hpp>
#include <marlin/core/fibers/MappingFiber.hpp>
#include <marlin/asyncio/udp/UdpFiber.hpp>
#include <marlin/stream/StreamFiber.hpp>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <fstream>
#include <boost/filesystem.hpp>

using namespace marlin::core;
using namespace marlin::asyncio;
using namespace marlin::stream;


#define l_SIZE 125000000
#define m_SIZE 125000
#define s_SIZE 125

#define SIZE l_SIZE

uint8_t static_sk[crypto_box_SECRETKEYBYTES];
uint8_t static_pk[crypto_box_PUBLICKEYBYTES];

struct Terminal {
	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = false;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	template<typename... Args>
	Terminal(Args&&...) {}

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
		} else if constexpr (tag == "did_recv"_tag) {
			return did_recv(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_recv_skip_stream"_tag) {
			return did_recv_skip_stream(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_recv_flush_stream"_tag) {
			return did_recv_flush_stream(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "did_recv_flush_conf"_tag) {
			return did_recv_flush_conf(std::forward<decltype(args)>(args)...);
		} else {
			static_assert(tag < 0);
		}
	}

	// template<uint32_t tag>
	// auto inner_call(auto&&... args) {
	// 	if constexpr (tag == "did_recv"_tag) {
	// 		return did_recv(std::forward<decltype(args)>(args)...);
	// 	} else if constexpr (tag == "did_dial"_tag) {
	// 		return did_dial(std::forward<decltype(args)>(args)...);
	// 	} else if constexpr (tag == "did_send"_tag) {
	// 		return did_send(std::forward<decltype(args)>(args)...);
	// 	} else if constexpr (tag == "did_close"_tag) {
	// 		return did_close(std::forward<decltype(args)>(args)...);
	// 	} else if constexpr (tag == "did_recv"_tag) {
	// 		return did_recv(std::forward<decltype(args)>(args)...);
	// 	} else {
	// 		static_assert(tag < 0);
	// 	}
	// }

private:
	int did_recv(
		auto&& src [[maybe_unused]],
		Buffer &&packet [[maybe_unused]],
		uint8_t stream_id [[maybe_unused]],
		SocketAddress
	) {
		// SPDLOG_INFO(
		// 	"Transport {{ Src: {}, Dst: {} }}: Did recv packet: {} bytes",
		// 	transport.src_addr.to_string(),
		// 	transport.dst_addr.to_string(),
		// 	packet.size()
		// );
		return 0;
	}

	void did_send(auto& transport, Buffer &&packet, SocketAddress addr) {
		SPDLOG_INFO(
			"Transport {{ Src: {}, Dst: {} }}: Did send packet: {} bytes",
			// transport.src_addr.to_string(),
			"",
			// transport.dst_addr.to_string(),
			"",
			packet.size()
		);
		did_dial(transport, addr);
	}

	int did_dial(auto& transport, SocketAddress addr) {
		auto buf = Buffer(SIZE);
		std::memset(buf.data(), 0, SIZE);

		SPDLOG_INFO("Did dial");

		return transport.template inner_call<"send"_tag>(*this, std::move(buf), addr, 0);
	}

	void did_close(auto&, SocketAddress, uint16_t = 0) {}

	void did_recv_flush_stream(
		auto& transport [[maybe_unused]],
		uint16_t stream_id [[maybe_unused]],
		uint64_t offset [[maybe_unused]],
		uint64_t old_offset [[maybe_unused]]
	) {}

	void did_recv_skip_stream(
		auto& transport [[maybe_unused]],
		uint16_t stream_id [[maybe_unused]]
	) {}

	void did_recv_flush_conf(
		auto& transport [[maybe_unused]],
		uint16_t stream_id [[maybe_unused]]
	) {}
};

int main() {
	if(sodium_init() == -1) {
		throw;
	}

	if(boost::filesystem::exists("./.marlin/keys/static")) {
		std::ifstream sk("./.marlin/keys/static", std::ios::binary);
		if(!sk.read((char *)static_sk, crypto_box_SECRETKEYBYTES)) {
			throw;
		}
		crypto_scalarmult_base(static_pk, static_sk);
	} else {
		crypto_box_keypair(static_pk, static_sk);

		boost::filesystem::create_directories("./.marlin/keys/");
		std::ofstream sk("./.marlin/keys/static", std::ios::binary);

		sk.write((char *)static_sk, crypto_box_SECRETKEYBYTES);
	}

	SPDLOG_INFO(
		"PK: {:spn}\nSK: {:spn}",
		spdlog::to_hex(static_pk, static_pk+32),
		spdlog::to_hex(static_sk, static_sk+32)
	);

	Fabric<
		Terminal,
		UdpFiber,
		MappingFiberF<StreamFiber, SocketAddress, uint8_t const*>::type
	> c(std::make_tuple(
		// terminal
		std::make_tuple(),
		// udp fiber
		std::make_tuple(),
		std::make_tuple(static_sk)
	));

	c.template outer_call<"bind"_tag>(0, SocketAddress::from_string("127.0.0.1:7000"));
	c.template outer_call<"dial"_tag>(0, SocketAddress::from_string("127.0.0.1:8000"), static_pk);

	return EventLoop::run();
}
