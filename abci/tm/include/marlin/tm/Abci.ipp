#include <rapidjson/document.h>

namespace marlin {
namespace tm {

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
	if (bytes.size() >= 9) {
		uint64_t response_id = ((uint64_t)(bytes.data()[0])<<56 | 
								(uint64_t)(bytes.data()[1])<<48 | 
								(uint64_t)(bytes.data()[2])<<40 | 
								(uint64_t)(bytes.data()[3])<<32 |
								(uint64_t)(bytes.data()[4])<<24 | 
								(uint64_t)(bytes.data()[5])<<16 | 
								(uint64_t)(bytes.data()[6])<<8  | 
								(uint64_t)(bytes.data()[7])      ); // BE decode
		bool verdict = bool(bytes.data()[9]);

		auto iter = block_store.find(response_id);
		if(iter == block_store.end()) {
			// Unknown request
			SPDLOG_ERROR("Abci: Unknown request");
			return;
		}

		auto block_request = std::move(iter->second);
		block_store.erase(iter);

		if (verdict) {
			std::apply([&](core::Buffer&& block, auto&& ...metadata) {

			delegate->did_analyze_block(
				*this,
				std::move(block),
				"",
				"",
				core::WeakBuffer((uint8_t*)"", 0),
				metadata...
			);
			}, std::move(block_request));
		}
	}
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
	// std::string rpc = "{\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":[],\"id\":0}";

	// pipe.send(core::WeakBuffer((uint8_t*)rpc.data(), rpc.size()));
}

template<ABCI_TEMPLATE>
template<typename... MT>
uint64_t ABCI::analyze_block(core::Buffer&& block, MT&&... metadata) {
	std::vector rpc_buffer(8+block.size(), 0);
	// uint8_t rpc_buffer[8 + block.size()];
	uint8_t idarr[8] = {
		(uint8_t)((id & 0xff00000000000000)>>56),
		(uint8_t)((id & 0x00ff000000000000)>>48),
		(uint8_t)((id & 0x0000ff0000000000)>>40),
		(uint8_t)((id & 0x000000ff00000000)>>32),
		(uint8_t)((id & 0x00000000ff000000)>>24),
		(uint8_t)((id & 0x0000000000ff0000)>>16),
		(uint8_t)((id & 0x000000000000ff00)>>8 ),
		(uint8_t)((id & 0x00000000000000ff)    )
		}; // BE encode

	std::copy(idarr, idarr+8, rpc_buffer.begin());
	std::copy(block.data(), block.data()+block.size(), rpc_buffer.begin()+8);

	pipe.send(core::WeakBuffer((uint8_t*)rpc_buffer.data(), 8+block.size()));

	block_store.try_emplace(id, std::move(block), std::forward<MT>(metadata)...);
	return id++;
}

//---------------- Helper macros undef begin ----------------//

#undef ABCI_TEMPLATE
#undef ABCI

//---------------- Helper macros undef end ----------------//

}  // namespace tm
}  // namespace marlin
