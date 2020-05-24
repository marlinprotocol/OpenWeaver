#ifndef MARLIN_STREAM_MESSAGES_HPP
#define MARLIN_STREAM_MESSAGES_HPP

#include <marlin/core/Buffer.hpp>


namespace marlin {
namespace stream {

template<typename BaseMessageType>
struct ConnIdMixin {
	BaseMessageType& set_src_conn_id(uint32_t src_conn_id) & {
		((BaseMessageType*)this)->base.payload_buffer().write_uint32_be_unsafe(2, src_conn_id);

		return *((BaseMessageType*)this);
	}

	BaseMessageType&& set_src_conn_id(uint32_t src_conn_id) && {
		return std::move(set_src_conn_id(src_conn_id));
	}

	uint32_t src_conn_id() const {
		return ((BaseMessageType*)this)->base.payload_buffer().read_uint32_be_unsafe(6);
	}

	BaseMessageType& set_dst_conn_id(uint32_t dst_conn_id) & {
		((BaseMessageType*)this)->base.payload_buffer().write_uint32_be_unsafe(6, dst_conn_id);

		return *((BaseMessageType*)this);
	}

	BaseMessageType&& set_dst_conn_id(uint32_t dst_conn_id) && {
		return std::move(set_dst_conn_id(dst_conn_id));
	}

	uint32_t dst_conn_id() const {
		return ((BaseMessageType*)this)->base.payload_buffer().read_uint32_be_unsafe(2);
	}
};

template<typename BaseMessageType>
struct DATAWrapper : public ConnIdMixin<DATAWrapper<BaseMessageType>> {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	[[nodiscard]] bool validate(size_t payload_size) const {
		return base.payload_buffer().size() >= 30 + payload_size;
	}

	DATAWrapper(size_t payload_size, bool is_fin) : base(30 + payload_size) {
		base.set_payload({0, static_cast<uint8_t>(is_fin)});
	}

	DATAWrapper(core::Buffer&& buf) : base(std::move(buf)) {}

	DATAWrapper& set_stream_id(uint16_t stream_id) & {
		base.payload_buffer().write_uint16_be_unsafe(18, stream_id);

		return *this;
	}

	DATAWrapper&& set_stream_id(uint16_t stream_id) && {
		return std::move(set_stream_id(stream_id));
	}

	uint16_t stream_id() const {
		return base.payload_buffer().read_uint16_be_unsafe(18);
	}

	DATAWrapper& set_packet_number(uint64_t packet_number) & {
		base.payload_buffer().write_uint64_be_unsafe(10, packet_number);

		return *this;
	}

	DATAWrapper&& set_packet_number(uint64_t packet_number) && {
		return std::move(set_packet_number(packet_number));
	}

	uint64_t packet_number() const {
		return base.payload_buffer().read_uint64_be_unsafe(10);
	}

	DATAWrapper& set_offset(uint64_t offset) & {
		base.payload_buffer().write_uint64_be_unsafe(20, offset);

		return *this;
	}

	DATAWrapper&& set_offset(uint64_t offset) && {
		return std::move(set_offset(offset));
	}

	uint64_t offset() const {
		return base.payload_buffer().read_uint64_be_unsafe(20);
	}

	DATAWrapper& set_length(uint16_t length) & {
		base.payload_buffer().write_uint16_be_unsafe(28, length);

		return *this;
	}

	DATAWrapper&& set_length(uint16_t length) && {
		return std::move(set_length(length));
	}

	uint16_t length() const {
		return base.payload_buffer().read_uint16_be_unsafe(28);
	}

	DATAWrapper& set_payload(uint8_t const* in, size_t size) & {
		base.payload_buffer().write_unsafe(30, in, size);

		return *this;
	}

	DATAWrapper&& set_payload(uint8_t const* in, size_t size) && {
		return std::move(set_payload(in, size));
	}

	DATAWrapper& set_payload(std::initializer_list<uint8_t> il) & {
		base.payload_buffer().write_unsafe(30, il.begin(), il.size());

		return *this;
	}

	DATAWrapper&& set_payload(std::initializer_list<uint8_t> il) && {
		return std::move(set_payload(il));
	}

	core::WeakBuffer payload_buffer() const& {
		auto buf = base.payload_buffer();
		buf.cover_unsafe(30);

		return buf;
	}

	core::Buffer payload_buffer() && {
		auto buf = std::move(base).payload_buffer();
		buf.cover_unsafe(30);

		return std::move(buf);
	}

	uint8_t* payload() {
		return base.payload_buffer().data() + 30;
	}

	bool is_fin_set() const {
		return base.payload_buffer().read_uint8_unsafe(1) == 1;
	}
};

template<typename BaseMessageType>
struct ACKWrapper : public ConnIdMixin<ACKWrapper<BaseMessageType>> {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	[[nodiscard]] bool validate() const {
		if(base.payload_buffer().size() < 20 || base.payload_buffer().size() != 20 + size()*8) {
			return false;
		}

		return true;
	}

	ACKWrapper(size_t num_ranges) : base(20 + 8*num_ranges) {
		base.set_payload({0, 2});
	}

	ACKWrapper(core::Buffer&& buf) : base(std::move(buf)) {}

	ACKWrapper& set_size(uint16_t size) & {
		base.payload_buffer().write_uint16_be_unsafe(10, size);

		return *this;
	}

	ACKWrapper&& set_size(uint16_t size) && {
		return std::move(set_size(size));
	}

	uint16_t size() const {
		return base.payload_buffer().read_uint16_be_unsafe(10);
	}

	ACKWrapper& set_packet_number(uint64_t packet_number) & {
		base.payload_buffer().write_uint64_be_unsafe(12, packet_number);

		return *this;
	}

	ACKWrapper&& set_packet_number(uint64_t packet_number) && {
		return std::move(set_packet_number(packet_number));
	}

	uint64_t packet_number() const {
		return base.payload_buffer().read_uint64_be_unsafe(12);
	}

	struct iterator {
	private:
		core::WeakBuffer buf;
		size_t offset = 0;
	public:
		// For iterator_traits
		using difference_type = int32_t;
		using value_type = uint64_t;
		using pointer = value_type const*;
		using reference = value_type const&;
		using iterator_category = std::input_iterator_tag;

		iterator(core::WeakBuffer buf, size_t offset = 0) : buf(buf), offset(offset) {}

		value_type operator*() const {
			return buf.read_uint64_be_unsafe(offset);
		}

		iterator& operator++() {
			offset += 8;

			return *this;
		}

		bool operator==(iterator const& other) const {
			return offset == other.offset;
		}

		bool operator!=(iterator const& other) const {
			return !(*this == other);
		}
	};

	iterator ranges_begin() const {
		return iterator(base.payload_buffer(), 20);
	}

	iterator ranges_end() const {
		return iterator(base.payload_buffer(), 20 + size()*8);
	}

	template<typename It>
	ACKWrapper& set_ranges(It begin, It end) & {
		size_t idx = 20;
		while(begin != end && idx + 8 <= base.payload_buffer().size()) {
			base.payload_buffer().write_uint64_be_unsafe(idx, *begin);
			idx += 8;
			++begin;
		}
		base.truncate_unsafe(base.payload_buffer().size() - idx);

		return *this;
	}

	template<typename It>
	ACKWrapper&& set_ranges(It begin, It end) && {
		return std::move(set_ranges(begin, end));
	}
};

template<typename BaseMessageType>
struct DIALWrapper : public ConnIdMixin<DIALWrapper<BaseMessageType>> {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	[[nodiscard]] bool validate(size_t payload_size) const {
		return base.payload_buffer().size() >= 10 + payload_size;
	}

	DIALWrapper(size_t payload_size) : base(10 + payload_size) {
		base.set_payload({0, 3});
	}

	DIALWrapper(core::Buffer&& buf) : base(std::move(buf)) {}

	DIALWrapper& set_payload(uint8_t const* in, size_t size) & {
		base.payload_buffer().write_unsafe(10, in, size);

		return *this;
	}

	DIALWrapper&& set_payload(uint8_t const* in, size_t size) && {
		return std::move(set_payload(in, size));
	}

	DIALWrapper& set_payload(std::initializer_list<uint8_t> il) & {
		base.payload_buffer().write_unsafe(10, il.begin(), il.size());

		return *this;
	}

	DIALWrapper&& set_payload(std::initializer_list<uint8_t> il) && {
		return std::move(set_payload(il));
	}

	uint8_t* payload() {
		return base.payload_buffer().data() + 10;
	}
};

template<typename BaseMessageType>
struct DIALCONFWrapper : public ConnIdMixin<DIALCONFWrapper<BaseMessageType>> {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	[[nodiscard]] bool validate(size_t payload_size) const {
		return base.payload_buffer().size() >= 10 + payload_size;
	}

	DIALCONFWrapper(size_t payload_size) : base(10 + payload_size) {
		base.set_payload({0, 4});
	}

	DIALCONFWrapper(core::Buffer&& buf) : base(std::move(buf)) {}

	DIALCONFWrapper& set_payload(uint8_t const* in, size_t size) & {
		base.payload_buffer().write_unsafe(10, in, size);

		return *this;
	}

	DIALCONFWrapper&& set_payload(uint8_t const* in, size_t size) && {
		return std::move(set_payload(in, size));
	}

	DIALCONFWrapper& set_payload(std::initializer_list<uint8_t> il) & {
		base.payload_buffer().write_unsafe(10, il.begin(), il.size());

		return *this;
	}

	DIALCONFWrapper&& set_payload(std::initializer_list<uint8_t> il) && {
		return std::move(set_payload(il));
	}

	uint8_t* payload() {
		return base.payload_buffer().data() + 10;
	}
};

template<typename BaseMessageType>
struct CONFWrapper : public ConnIdMixin<CONFWrapper<BaseMessageType>> {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	[[nodiscard]] bool validate() const {
		return base.payload_buffer().size() >= 10;
	}

	CONFWrapper() : base(10) {
		base.set_payload({0, 5});
	}

	CONFWrapper(core::Buffer&& buf) : base(std::move(buf)) {}
};

template<typename BaseMessageType>
struct RSTWrapper : public ConnIdMixin<RSTWrapper<BaseMessageType>> {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	[[nodiscard]] bool validate() const {
		return base.payload_buffer().size() >= 10;
	}

	RSTWrapper() : base(10) {
		base.set_payload({0, 6});
	}

	RSTWrapper(core::Buffer&& buf) : base(std::move(buf)) {}
};

template<typename BaseMessageType>
struct SKIPSTREAMWrapper : public ConnIdMixin<SKIPSTREAMWrapper<BaseMessageType>> {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	[[nodiscard]] bool validate() const {
		return base.payload_buffer().size() >= 20;
	}

	SKIPSTREAMWrapper() : base(20) {
		base.set_payload({0, 7});
	}

	SKIPSTREAMWrapper(core::Buffer&& buf) : base(std::move(buf)) {}

	SKIPSTREAMWrapper& set_stream_id(uint16_t stream_id) & {
		base.payload_buffer().write_uint16_be_unsafe(10, stream_id);

		return *this;
	}

	SKIPSTREAMWrapper&& set_stream_id(uint16_t stream_id) && {
		return std::move(set_stream_id(stream_id));
	}

	uint16_t stream_id() const {
		return base.payload_buffer().read_uint16_be_unsafe(10);
	}

	SKIPSTREAMWrapper& set_offset(uint64_t offset) & {
		base.payload_buffer().write_uint64_be_unsafe(12, offset);

		return *this;
	}

	SKIPSTREAMWrapper&& set_offset(uint64_t offset) && {
		return std::move(set_offset(offset));
	}

	uint64_t offset() const {
		return base.payload_buffer().read_uint64_be_unsafe(12);
	}
};

template<typename BaseMessageType>
struct FLUSHSTREAMWrapper : public ConnIdMixin<FLUSHSTREAMWrapper<BaseMessageType>> {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	[[nodiscard]] bool validate() const {
		return base.payload_buffer().size() >= 20;
	}

	FLUSHSTREAMWrapper() : base(20) {
		base.set_payload({0, 8});
	}

	FLUSHSTREAMWrapper(core::Buffer&& buf) : base(std::move(buf)) {}

	FLUSHSTREAMWrapper& set_stream_id(uint16_t stream_id) & {
		base.payload_buffer().write_uint16_be_unsafe(10, stream_id);

		return *this;
	}

	FLUSHSTREAMWrapper&& set_stream_id(uint16_t stream_id) && {
		return std::move(set_stream_id(stream_id));
	}

	uint16_t stream_id() const {
		return base.payload_buffer().read_uint16_be_unsafe(10);
	}

	FLUSHSTREAMWrapper& set_offset(uint64_t offset) & {
		base.payload_buffer().write_uint64_be_unsafe(12, offset);

		return *this;
	}

	FLUSHSTREAMWrapper&& set_offset(uint64_t offset) && {
		return std::move(set_offset(offset));
	}

	uint64_t offset() const {
		return base.payload_buffer().read_uint64_be_unsafe(12);
	}
};

template<typename BaseMessageType>
struct FLUSHCONFWrapper : public ConnIdMixin<FLUSHCONFWrapper<BaseMessageType>> {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	[[nodiscard]] bool validate() const {
		return base.payload_buffer().size() >= 12;
	}

	FLUSHCONFWrapper() : base(12) {
		base.set_payload({0, 9});
	}

	FLUSHCONFWrapper(core::Buffer&& buf) : base(std::move(buf)) {}

	FLUSHCONFWrapper& set_stream_id(uint16_t stream_id) & {
		base.payload_buffer().write_uint16_be_unsafe(10, stream_id);

		return *this;
	}

	FLUSHCONFWrapper&& set_stream_id(uint16_t stream_id) && {
		return std::move(set_stream_id(stream_id));
	}

	uint16_t stream_id() const {
		return base.payload_buffer().read_uint16_be_unsafe(10);
	}
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_MESSAGES_HPP
