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
		uint64_t id = bytes.read_uint64_be_unsafe(0);
		bool verdict = bool(bytes.data()[8]);

		auto iter = block_store.find(id);
		if(iter == block_store.end()) {
			// Unknown request
			SPDLOG_ERROR("Abci: Unknown request");
			return;
		}

		auto block_request = std::move(iter->second);
		block_store.erase(iter);

		if (verdict) {
			std::apply([&](core::Buffer&& block, auto&& ...metadata) {
			core::WeakBuffer header = block;

			delegate->did_analyze_block(
				*this,
				std::move(block),
				"",
				"",
				header,
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
void ABCI::get_block_number() {}

template<ABCI_TEMPLATE>
template<typename... MT>
uint64_t ABCI::analyze_block(core::Buffer&& block, MT&&... metadata) {
	auto b_size = block.size();
	core::Buffer request_message(b_size + 16);

	request_message.write_uint64_be_unsafe(0, id);
	request_message.write_uint64_be_unsafe(8, b_size);
	request_message.write_unsafe(16, block.data(), b_size);

	pipe.send(core::WeakBuffer((uint8_t*)request_message.data(), 16+block.size()));

	block_store.try_emplace(id, std::move(block), std::forward<MT>(metadata)...);
	return id++;
}

//---------------- Helper macros undef begin ----------------//

#undef ABCI_TEMPLATE
#undef ABCI

//---------------- Helper macros undef end ----------------//

}  // namespace tm
}  // namespace marlin
