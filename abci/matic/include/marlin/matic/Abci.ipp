#include <rapidjson/document.h>

namespace marlin {
namespace matic {

//---------------- Helper macros begin ----------------//

#define ABCI_TEMPLATE typename DelegateType, typename... MetadataTypes

#define ABCI Abci< \
	DelegateType, \
	MetadataTypes... \
>

//---------------- Helper macros end ----------------//

template<ABCI_TEMPLATE>
int ABCI::did_connect(FiberType&) {
	fiber.o(*this).reset(1000);
	connect_timer_interval = 1000;
	delegate->did_connect(*this);
	return 0;
}

template<ABCI_TEMPLATE>
int ABCI::did_recv(FiberType&, core::Buffer&& buf, core::SocketAddress addr [[maybe_unused]]) {
	SPDLOG_DEBUG("{}", std::string((char*)buf.data(), buf.size()));
	if(state == 0) {
		// headers
		if(buf.size() == 2) {
			// empty
			if(length == 0) {
				// still do not have length
				SPDLOG_ERROR("RPC resp error: no length");
				fiber.o(*this).close();
				return -1;
			}

			// go to next phase
			state = 1;
			fiber.o(*this).template transform<1>(std::make_tuple(), std::make_tuple());
			fiber.o(*this).reset(length);
			return 0;
		} else if(buf.size() > 16 &&
					(std::memcmp(buf.data(), u8"Content-Length: ", 16) == 0 ||
					std::memcmp(buf.data(), u8"content-length: ", 16) == 0)) {
			// parse length
			auto length_str = std::string((char*)buf.data()+16, buf.size() - 18);
			length = std::stol(length_str);
		}
		fiber.o(*this).reset(1000);
	} else {
		// body
		state = 0;
		length = 0;
		SPDLOG_DEBUG("{}", std::string((char*)buf.data(), buf.size()));
		{
		rapidjson::Document d;
		d.Parse((char*)buf.data(), buf.size());
		fiber.o(*this).template transform<0>(std::make_tuple(), std::make_tuple());
		fiber.o(*this).reset(1000);

		if(!(
			!d.HasParseError() && d.IsObject() &&
			d.HasMember("id") && d["id"].IsUint64() && (
				(
					d.HasMember("error") && d["error"].IsObject() &&
					d["error"].HasMember("message") && d["error"]["message"].IsString()
				) || (
					d.HasMember("result") && d["result"].IsObject() &&
					d["result"].HasMember("hash") && d["result"]["hash"].IsString() &&
					d["result"].HasMember("headerOffset") && d["result"]["headerOffset"].IsUint64() &&
					d["result"].HasMember("headerLength") && d["result"]["headerLength"].IsUint64() &&
					d["result"].HasMember("coinbase") && d["result"]["coinbase"].IsString()
				)
			)
		)) {
			// Failed to validate, shouldn't happen!!!
			SPDLOG_ERROR("Abci: Failed to validate response");
			return 0;
		}

		if(d.HasMember("error")) {
			// RPC error
			SPDLOG_ERROR(
				"Abci: RPC error: {}",
				std::string(d["error"]["message"].GetString(), d["error"]["message"].GetStringLength())
			);
			block_store.erase(d["id"].GetUint64());
			return 0;
		}

		auto iter = block_store.find(d["id"].GetUint64());
		if(iter == block_store.end()) {
			// Unknown request
			SPDLOG_ERROR("Abci: Unknown request");
			return 0;
		}

		auto block_request = std::move(iter->second);
		block_store.erase(iter);

		std::apply([&](core::Buffer&& block, auto&& ...metadata) {
			core::WeakBuffer header(
				block.data() + d["result"]["headerOffset"].GetUint64(),
				d["result"]["headerLength"].GetUint64()
			);

			delegate->did_analyze_block(
				*this,
				std::move(block),
				std::string(d["result"]["hash"].GetString(), d["result"]["hash"].GetStringLength()),
				std::string(d["result"]["coinbase"].GetString(), d["result"]["coinbase"].GetStringLength()),
				header,
				metadata...
			);
		}, std::move(block_request));
		}
	}

	return 0;
}

template<ABCI_TEMPLATE>
int ABCI::did_disconnect(FiberType&, uint reason) {
	// Wait and retry
	connect_timer.template start<
		SelfType,
		&SelfType::connect_timer_cb
	>(connect_timer_interval, 0);

	// Exponential backoff till ~1 min
	connect_timer_interval *= 2;
	if(connect_timer_interval > 64000) {
		connect_timer_interval = 64000;
	}

	if(reason == 0) {  // Fresh disconnection, notify
		delegate->did_disconnect(*this);
	}

	return 0;
}

template<ABCI_TEMPLATE>
int ABCI::did_close(FiberType&) {
	delegate->did_close(*this);
	return 0;
}

template<ABCI_TEMPLATE>
void ABCI::get_block_number() {
	std::string rpc =
		"{"
			"\"jsonrpc\":\"2.0\","
			"\"method\":\"eth_blockNumber\","
			"\"params\":[],\"id\":0"
		"}";

	core::Buffer req(fmt::formatted_size(
		"POST / HTTP/1.1\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: {}\r\n"
		"\r\n"
		"{}",
		rpc.size(),
		rpc
	));
	fmt::format_to_n(
		req.data(),
		req.size(),
		"POST / HTTP/1.1\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: {}\r\n"
		"\r\n"
		"{}",
		rpc.size(),
		rpc
	);

	fiber.o(*this).send(*this, std::move(req));
}

template<ABCI_TEMPLATE>
template<typename... MT>
uint64_t ABCI::analyze_block(core::Buffer&& block, MT&&... metadata) {
	std::string hex_block("0x");
	hex_block.reserve(block.size()*2);

	for(size_t i = 1; i < block.size(); i++) {
		hex_block += fmt::format("{:02x}", block.data()[i]);
	}

	std::string rpc = fmt::format(
		"{{"
			"\"jsonrpc\":\"2.0\","
			"\"method\":\"lin_analyzeBlock\","
			"\"id\":{},\"params\":[\"{}\"]"
		"}}",
		id,
		hex_block
	);

	core::Buffer req(fmt::formatted_size(
		"POST / HTTP/1.1\r\n"
		"Host: 0.0.0.0\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: {}\r\n"
		"\r\n"
		"{}",
		rpc.size(),
		rpc
	));
	fmt::format_to_n(
		req.data(),
		req.size(),
		"POST / HTTP/1.1\r\n"
		"Host: 0.0.0.0\r\n"
		"Content-Type: application/json\r\n"
		"Content-Length: {}\r\n"
		"\r\n"
		"{}",
		rpc.size(),
		rpc
	);

	// SPDLOG_INFO("Sending for spam check: {}", req.size());

	fiber.o(*this).send(*this, std::move(req));

	block_store.try_emplace(id, std::move(block), std::forward<MT>(metadata)...);
	return id++;
}

//---------------- Helper macros undef begin ----------------//

#undef ABCI_TEMPLATE
#undef ABCI

//---------------- Helper macros undef end ----------------//

}  // namespace matic
}  // namespace marlin
