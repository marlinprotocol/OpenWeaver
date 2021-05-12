#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace marlin {
namespace dot {

//---------------- Helper macros begin ----------------//

#define ABCI_TEMPLATE typename DelegateType, typename... MetadataTypes

#define ABCI Abci< \
	DelegateType, \
	MetadataTypes... \
>

//---------------- Helper macros end ----------------//

template<ABCI_TEMPLATE>
void ABCI::did_connect(BaseTransport&) {
	connect_timer_interval = 1000;
	delegate->did_connect(*this);
}

template<ABCI_TEMPLATE>
void ABCI::did_recv(BaseTransport& transport, core::Buffer&& bytes) {
	std::string resp = std::string((const char *)bytes.data(), (const char*)bytes.data() + bytes.size());

	if(resp.find("200 OK") == std::string::npos || resp.find("content-length") == std::string::npos) {
		SPDLOG_ERROR("Abci: Invalid response");
		return;
	}

	int data_len = std::stoi(resp.substr(resp.find("content-length") + 16, resp.find("\n", resp.find("content-length") + 16) ));
	resp = resp.substr(resp.size() - data_len - 1);
	SPDLOG_DEBUG("data: {}", resp);
	rapidjson::Document d;
	d.Parse(resp.c_str(), resp.size());
	if(!(
		!d.HasParseError() && d.IsObject() &&
		d.HasMember("id") && d["id"].IsUint64() && (
			(
				d.HasMember("error") && d["error"].IsObject() &&
				d["error"].HasMember("message") && d["error"]["message"].IsString()
			) || (
				d.HasMember("result") && d["result"].IsBool()
			)
		)
	)) {
		// Failed to validate, shouldn't happen!!!
		SPDLOG_ERROR("Abci: Failed to validate response");
		return;
	}

	if(d.HasMember("error")) {
		// RPC error
		SPDLOG_ERROR(
			"Abci: RPC error: {}",
			std::string(d["error"]["message"].GetString(), d["error"]["message"].GetStringLength())
		);
		block_store.erase(d["id"].GetUint64());
		return;
	}

	if(bytes.size() == 0) {
		// do not have result
		return;
	}

	auto id = d["id"].GetUint64();
	bool res = d["result"].GetBool();
	auto iter = block_store.find(d["id"].GetUint64());
	if(iter == block_store.end()) {
		// Unknown request
		SPDLOG_ERROR("Abci: Unknown request");
		return;
	}
	if(!res) {
		// RPC error
		SPDLOG_ERROR(
			"Abci: RPC error: {}",
			res
		);
		block_store.erase(id);
		return;
	}

	auto block_request = std::move(iter->second);
	block_store.erase(iter);

	std::apply([&](core::Buffer&& block, auto&& ...metadata) {
		delegate->did_analyze_block(
			*this,
			std::move(block),
			"",
			"",
			core::WeakBuffer(nullptr, 0),
			metadata...
		);
	}, std::move(block_request));

	counter = 0;
	resp_id = 0;
	if(bytes.size() == 1) {
		return;
	}
	// bytes.cover_unsafe(1);
	// did_recv(transport, std::move(bytes));
}

template<ABCI_TEMPLATE>
void ABCI::did_disconnect(BaseTransport&, uint reason) {
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
}

template<ABCI_TEMPLATE>
void ABCI::did_close(BaseTransport&) {
	delegate->did_close(*this);
}

template<ABCI_TEMPLATE>
void ABCI::get_block_number() {
	//std::string rpc = "{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[],\"id\":0}";

	//tcp.send(core::WeakBuffer((uint8_t*)rpc.data(), rpc.size()));
}

template<ABCI_TEMPLATE>
template<typename... MT>
uint64_t ABCI::analyze_block(core::Buffer&& block, MT&&... metadata) {
	std::string hex_block;
	hex_block.reserve(block.size()*2);

	for(size_t i = 1; i < block.size(); i++) {
		hex_block += fmt::format("{:02x}", block.data()[i]);
	}
	SPDLOG_INFO("dot: hex block: {} {}", hex_block, hex_block.length());
	
	std::string rpc = fmt::format("{{\"jsonrpc\":\"2.0\",\"method\":\"spamCheck\",\"id\":{},\"params\":[\"{}\"]}}", id, hex_block);
	std::string http_header = fmt::format("POST / HTTP/1.1\r\nHost: {}\r\nContent-type: application/json;charset=utf-8\r\nContent-length: {}\r\n\r\n{}\r\n", this->dst.to_string(), rpc.size(), rpc );
	tcp.send(core::WeakBuffer((uint8_t*)http_header.data(), http_header.size()));
	
	block_store.try_emplace(id, std::move(block), std::forward<MT>(metadata)...);
	return id++;
}

//---------------- Helper macros undef begin ----------------//

#undef ABCI_TEMPLATE
#undef ABCI

//---------------- Helper macros undef end ----------------//

}  // namespace dot
}  // namespace marlin
