#include <rapidjson/document.h>

namespace marlin {
namespace near {

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

	SPDLOG_INFO("DSFSDFS");

	uint8_t buf[] = {0,1,86,174,128,222,41,234,208,241,92,137,199,53,20,142,220,143,118,194,226,111,45,117,136,188,43,175,75,32,93,4,103,105,137,208,1,0,0,0,0,0,70,7,13,116,225,154,151,12,245,176,110,187,174,206,245,12,114,127,224,254,204,181,182,34,249,78,231,232,38,145,103,111,52,83,190,51,38,120,239,180,52,27,52,103,12,64,36,68,105,54,214,251,14,214,89,255,88,60,9,179,8,212,52,97,89,190,180,237,149,228,36,94,27,239,83,231,194,83,27,178,61,238,117,44,226,171,222,152,115,11,124,152,252,41,91,45,102,104,122,173,248,98,189,119,108,143,193,139,142,159,142,32,8,151,20,133,110,226,51,179,144,42,89,29,13,95,41,37,33,134,2,70,39,85,123,22,134,54,150,66,58,223,224,6,181,144,120,49,57,106,49,79,7,172,217,126,86,33,204,45,159,23,255,187,197,149,182,198,29,113,211,177,86,164,95,147,216,175,36,241,239,141,151,36,121,189,50,205,216,77,244,114,104,234,96,113,246,190,44,173,122,79,222,54,164,38,222,175,94,18,82,245,113,138,6,225,120,127,141,18,203,125,165,161,128,87,250,159,251,102,206,13,130,28,227,150,32,132,154,59,153,175,140,207,160,156,234,53,49,178,117,87,207,30,60,49,229,5,66,207,210,154,26,35,102,104,122,173,248,98,189,119,108,143,193,139,142,159,142,32,8,151,20,133,110,226,51,179,144,42,89,29,13,95,41,37,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,211,68,189,112,144,2,214,114,13,196,32,36,63,204,86,77,199,185,129,1,99,95,252,188,97,118,186,54,254,45,245,146,0,0,0,0,1,0,0,0,1,0,202,154,59,0,0,0,0,0,0,0,0,0,0,0,0,233,58,250,164,178,217,208,202,79,86,222,127,23,101,0,0,0,0,0,0,208,156,254,222,240,245,56,117,105,220,116,221,115,164,237,234,229,181,123,198,79,69,198,182,149,88,226,137,7,171,65,78,86,174,128,222,41,234,208,241,92,137,199,53,20,142,220,143,118,194,226,111,45,117,136,188,43,175,75,32,93,4,103,105,1,0,0,0,1,0,7,208,216,46,125,8,65,65,187,131,128,226,164,223,209,169,64,4,28,130,26,210,131,94,251,55,235,38,60,169,198,151,91,58,213,169,32,126,167,137,33,71,58,114,100,89,137,20,135,183,55,218,1,160,65,250,171,200,200,62,30,97,246,10,38,0,0,0,0,90,246,93,185,68,197,40,106,241,71,237,45,237,228,139,30,249,192,176,169,176,186,147,92,155,39,162,7,35,56,118,98,242,63,32,57,51,80,70,65,195,105,244,113,165,113,12,143,224,27,228,128,22,48,220,202,248,114,242,218,8,113,195,6,1,0,0,0,86,174,128,222,41,234,208,241,92,137,199,53,20,142,220,143,118,194,226,111,45,117,136,188,43,175,75,32,93,4,103,105,235,160,122,141,7,33,101,130,129,125,85,173,93,93,120,93,112,210,136,75,86,101,248,33,214,241,131,79,174,2,255,68,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,91,63,18,82,35,126,6,206,3,190,149,33,100,237,244,203,102,150,144,86,11,75,126,59,129,133,143,3,229,43,114,88,8,0,0,0,0,0,0,0,137,208,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,128,198,164,126,141,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,238,155,146,227,36,224,52,26,150,93,175,41,179,149,85,3,14,44,14,85,145,215,211,226,189,5,255,120,95,148,107,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,137,208,1,0,0,0,0,0,0,135,36,75,195,189,241,7,5,29,186,244,59,204,195,73,44,120,107,41,164,202,23,99,241,104,2,199,234,242,107,8,5,192,7,28,86,246,26,194,80,99,200,206,140,87,172,40,113,20,156,159,108,246,14,177,147,42,233,81,224,78,82,83,9,0,0,0,0,176,132,106,113,117,56,169,243,196,210,247,204,147,41,240,136,180,242,194,73,42,10,185,221,100,35,57,240,42,151,7,44,186,54,175,128,29,117,178,21,127,38,205,178,182,108,79,4,114,185,112,117,174,45,101,24,253,4,6,250,80,228,228,10,208,167,141,254,106,110,119,180,93,189,143,24,136,21,250,144,86,134,251,138,13,30,202,58,75,214,106,47,193,151,186,13};
	core::Buffer block(buf, sizeof(buf));
	uint64_t x = 5;
	analyze_block(std::move(block), std::move(x));
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

	std::string block_bin = "";
	uint8_t *buf = block.data();
	for(int i = 0; i < int(block.size()); i++) {
		block_bin += std::to_string(buf[i]) + (i < int(block.size()) - 1 ? ", ": "");
	}
	buf = NULL;

	std::string rpcBody = "{"
		"\"jsonrpc\": \"2.0\","
		"\"id\": \"dontcare\","
		"\"method\": \"query\","
		"\"params\": {"
			"\"request_type\": \"dummy_function\","
			"\"account_id\": \"client.chainlink.testnet\","
			"\"finality\": \"final\","
			"\"block_bin\": [" + block_bin + "]"
		"}"
	"}";

	std::string rpc = "POST / HTTP/1.0\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(rpcBody.size()) + "\r\n\r\n" + rpcBody;

	core::Buffer req(rpc.size());
	req.write_unsafe(0, reinterpret_cast<const uint8_t*>(rpc.data()), rpc.size());
	tcp.send(std::move(req));

	block_store.try_emplace(id, std::move(block), std::forward<MT>(metadata)...);
	return id++;
}

//---------------- Helper macros undef begin ----------------//

#undef ABCI_TEMPLATE
#undef ABCI

//---------------- Helper macros undef end ----------------//

}  // namespace near
}  // namespace marlin
