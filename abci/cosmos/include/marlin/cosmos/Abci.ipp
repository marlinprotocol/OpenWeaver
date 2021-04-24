#include <rapidjson/document.h>

namespace marlin {
namespace cosmos {

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
	if(bytes.size() + counter < 8) {
		// Partial id
		for(uint8_t i = 0; i < bytes.size(); i++, counter++) {
			resp_id = (resp_id << 8) | bytes.data()[i];
		}
		return;
	} else {
		// Full size available
		uint8_t i = 0;
		for(; counter < 8; i++, counter++) {
			resp_id = (resp_id << 8) | bytes.data()[i];
		}
		bytes.cover_unsafe(i);
	}

	if(bytes.size() == 0) {
		// do not have result
		return;
	}

	auto id = resp_id;
	bool res = bytes.read_uint8_unsafe(0) > 0;

	if(!res) {
		// RPC error
		SPDLOG_ERROR(
			"Abci: RPC error: {}",
			res
		);
		block_store.erase(id);
		return;
	}

	auto iter = block_store.find(id);
	if(iter == block_store.end()) {
		// Unknown request
		SPDLOG_ERROR("Abci: Unknown request");
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
	bytes.cover_unsafe(1);
	did_recv(transport, std::move(bytes));
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
	core::Buffer req(block.size() + 16);
	req.write_uint64_be_unsafe(0, id);
	req.write_uint64_be_unsafe(8, block.size());
	req.write_unsafe(16, block.data(), block.size());
	tcp.send(req);

	block_store.try_emplace(id, std::move(block), std::forward<MT>(metadata)...);
	return id++;
}

//---------------- Helper macros undef begin ----------------//

#undef ABCI_TEMPLATE
#undef ABCI

//---------------- Helper macros undef end ----------------//

}  // namespace cosmos
}  // namespace marlin
