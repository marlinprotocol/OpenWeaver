#include <rapidjson/document.h>

namespace marlin {
namespace eth {

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
void ABCI::did_recv(BaseTransport&, core::Buffer&& bytes) {
	// TODO: Need to do framing, most implementations seem to use newline as delimiter
	rapidjson::Document d;
	d.Parse((char*)bytes.data(), bytes.size());

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

	auto iter = block_store.find(d["id"].GetUint64());
	if(iter == block_store.end()) {
		// Unknown request
		SPDLOG_ERROR("Abci: Unknown request");
		return;
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
	std::string rpc = "{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[],\"id\":0}";

	pipe.send(core::WeakBuffer((uint8_t*)rpc.data(), rpc.size()));
}

template<ABCI_TEMPLATE>
template<typename... MT>
uint64_t ABCI::analyze_block(core::Buffer&& block, MT&&... metadata) {
	std::string hex_block("0x");
	hex_block.reserve(block.size()*2);

	for(size_t i = 1; i < block.size(); i++) {
		hex_block += fmt::format("{:02x}", block.data()[i]);
	}

	std::string rpc = fmt::format("{{\"jsonrpc\":\"2.0\",\"method\":\"lin_analyzeBlock\",\"id\":{},\"params\":[\"{}\"]}}", id, hex_block);

	pipe.send(core::WeakBuffer((uint8_t*)rpc.data(), rpc.size()));

	block_store.try_emplace(id, std::move(block), std::forward<MT>(metadata)...);
	return id++;
}

//---------------- Helper macros undef begin ----------------//

#undef ABCI_TEMPLATE
#undef ABCI

//---------------- Helper macros undef end ----------------//

}  // namespace eth
}  // namespace marlin
