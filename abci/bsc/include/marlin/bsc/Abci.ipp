#include <rapidjson/document.h>

namespace marlin {
namespace bsc {

//---------------- Helper macros begin ----------------//

#define ABCI_TEMPLATE typename DelegateType

#define ABCI Abci< \
	DelegateType \
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

	core::Buffer block = std::move(iter->second);
	block_store.erase(iter);

	core::WeakBuffer header(
		block.data() + d["result"]["headerOffset"].GetUint64(),
		d["result"]["headerLength"].GetUint64()
	);

	delegate->did_analyze_block(
		*this,
		std::move(block),
		std::string(d["result"]["hash"].GetString(), d["result"]["hash"].GetStringLength()),
		std::string(d["result"]["coinbase"].GetString(), d["result"]["coinbase"].GetStringLength()),
		header
	);
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

//---------------- Helper macros undef begin ----------------//

#undef ABCI_TEMPLATE
#undef ABCI

//---------------- Helper macros undef end ----------------//

}  // namespace bsc
}  // namespace marlin
