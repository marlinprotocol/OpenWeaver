#ifndef MARLIN_STREAM_MESSAGES_HPP
#define MARLIN_STREAM_MESSAGES_HPP

#include <marlin/core/Buffer.hpp>


namespace marlin {
namespace stream {

#define MARLIN_MESSAGES_UINT_FIELD_MULTIPLE_OFFSET(size, name, read_offset, write_offset) \
	SelfType& set_##name(uint##size##_t name) & { \
		base.payload_buffer().write_uint##size##_le_unsafe(write_offset, name); \
 \
		return *this; \
	} \
 \
	SelfType&& set_##name(uint##size##_t name) && { \
		return std::move(set_##name(name)); \
	} \
 \
	uint##size##_t name() const { \
		return base.payload_buffer().read_uint##size##_le_unsafe(read_offset); \
	}

#define MARLIN_MESSAGES_UINT_FIELD_SINGLE_OFFSET(size, name, offset) MARLIN_MESSAGES_UINT_FIELD_MULTIPLE_OFFSET(size, name, offset, offset)

#define MARLIN_MESSAGES_GET_4(_0, _1, _2, _3, MACRO, ...) MACRO
#define MARLIN_MESSAGES_GET_UINT_FIELD(...) MARLIN_MESSAGES_GET_4(__VA_ARGS__, MARLIN_MESSAGES_UINT_FIELD_MULTIPLE_OFFSET, MARLIN_MESSAGES_UINT_FIELD_SINGLE_OFFSET, throw)
#define MARLIN_MESSAGES_UINT_FIELD(...) MARLIN_MESSAGES_GET_UINT_FIELD(__VA_ARGS__)(__VA_ARGS__)

#define MARLIN_MESSAGES_PAYLOAD_FIELD(offset) \
	SelfType& set_payload(uint8_t const* in, size_t size) & { \
		base.payload_buffer().write_unsafe(offset, in, size); \
 \
		return *this; \
	} \
 \
	SelfType&& set_payload(uint8_t const* in, size_t size) && { \
		return std::move(set_payload(in, size)); \
	} \
 \
	SelfType& set_payload(std::initializer_list<uint8_t> il) & { \
		base.payload_buffer().write_unsafe(offset, il.begin(), il.size()); \
 \
		return *this; \
	} \
 \
	SelfType&& set_payload(std::initializer_list<uint8_t> il) && { \
		return std::move(set_payload(il)); \
	} \
 \
	core::WeakBuffer payload_buffer() const& { \
		auto buf = base.payload_buffer(); \
		buf.cover_unsafe(offset); \
 \
		return buf; \
	} \
 \
	core::Buffer payload_buffer() && { \
		auto buf = std::move(base).payload_buffer(); \
		buf.cover_unsafe(offset); \
 \
		return std::move(buf); \
	} \
 \
	uint8_t* payload() { \
		return base.payload_buffer().data() + offset; \
	}


template<typename BaseMessageType>
struct DATAWrapper {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	using SelfType = DATAWrapper<BaseMessageType>;

	[[nodiscard]] bool validate(size_t payload_size) const {
		return base.payload_buffer().size() >= 30 + payload_size;
	}

	DATAWrapper(size_t payload_size, bool is_fin) : base(30 + payload_size) {
		base.set_payload({0, static_cast<uint8_t>(is_fin)});
	}

	DATAWrapper(BaseMessageType&& base) : base(std::move(base)) {}

	MARLIN_MESSAGES_UINT_FIELD(32, src_conn_id, 6, 2)
	MARLIN_MESSAGES_UINT_FIELD(32, dst_conn_id, 2, 6)
	MARLIN_MESSAGES_UINT_FIELD(64, packet_number, 10)
	MARLIN_MESSAGES_UINT_FIELD(16, stream_id, 18)
	MARLIN_MESSAGES_UINT_FIELD(64, offset, 20)
	MARLIN_MESSAGES_UINT_FIELD(16, length, 22)
	MARLIN_MESSAGES_PAYLOAD_FIELD(30)

	bool is_fin_set() const {
		return base.payload_buffer().read_uint8_unsafe(1) == 1;
	}
};

template<typename BaseMessageType>
struct ACKWrapper {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	using SelfType = ACKWrapper<BaseMessageType>;

	[[nodiscard]] bool validate() const {
		if(base.payload_buffer().size() < 20 || base.payload_buffer().size() != 20 + size()*8) {
			return false;
		}

		return true;
	}

	ACKWrapper(size_t num_ranges) : base(20 + 8*num_ranges) {
		base.set_payload({0, 2});
	}

	ACKWrapper(BaseMessageType&& base) : base(std::move(base)) {}

	MARLIN_MESSAGES_UINT_FIELD(32, src_conn_id, 6, 2)
	MARLIN_MESSAGES_UINT_FIELD(32, dst_conn_id, 2, 6)
	MARLIN_MESSAGES_UINT_FIELD(16, size, 10)
	MARLIN_MESSAGES_UINT_FIELD(64, packet_number, 12)

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
			return buf.read_uint64_le_unsafe(offset);
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
			base.payload_buffer().write_uint64_le_unsafe(idx, *begin);
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
struct DIALWrapper {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	using SelfType = DIALWrapper<BaseMessageType>;

	[[nodiscard]] bool validate(size_t payload_size) const {
		return base.payload_buffer().size() >= 10 + payload_size;
	}

	DIALWrapper(size_t payload_size) : base(10 + payload_size) {
		base.set_payload({0, 3});
	}

	DIALWrapper(BaseMessageType&& base) : base(std::move(base)) {}

	MARLIN_MESSAGES_UINT_FIELD(32, src_conn_id, 6, 2)
	MARLIN_MESSAGES_UINT_FIELD(32, dst_conn_id, 2, 6)
	MARLIN_MESSAGES_PAYLOAD_FIELD(10)
};

template<typename BaseMessageType>
struct DIALCONFWrapper {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	using SelfType = DIALCONFWrapper<BaseMessageType>;

	[[nodiscard]] bool validate(size_t payload_size) const {
		return base.payload_buffer().size() >= 10 + payload_size;
	}

	DIALCONFWrapper(size_t payload_size) : base(10 + payload_size) {
		base.set_payload({0, 4});
	}

	DIALCONFWrapper(BaseMessageType&& base) : base(std::move(base)) {}

	MARLIN_MESSAGES_UINT_FIELD(32, src_conn_id, 6, 2)
	MARLIN_MESSAGES_UINT_FIELD(32, dst_conn_id, 2, 6)
	MARLIN_MESSAGES_PAYLOAD_FIELD(10)
};

template<typename BaseMessageType>
struct CONFWrapper {
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

	CONFWrapper(BaseMessageType&& base) : base(std::move(base)) {}

	using SelfType = CONFWrapper<BaseMessageType>;

	MARLIN_MESSAGES_UINT_FIELD(32, src_conn_id, 6, 2)
	MARLIN_MESSAGES_UINT_FIELD(32, dst_conn_id, 2, 6)
};

template<typename BaseMessageType>
struct RSTWrapper {
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

	RSTWrapper(BaseMessageType&& base) : base(std::move(base)) {}

	using SelfType = RSTWrapper<BaseMessageType>;

	MARLIN_MESSAGES_UINT_FIELD(32, src_conn_id, 6, 2)
	MARLIN_MESSAGES_UINT_FIELD(32, dst_conn_id, 2, 6)
};

template<typename BaseMessageType>
struct SKIPSTREAMWrapper {
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

	SKIPSTREAMWrapper(BaseMessageType&& base) : base(std::move(base)) {}

	using SelfType = SKIPSTREAMWrapper<BaseMessageType>;

	MARLIN_MESSAGES_UINT_FIELD(32, src_conn_id, 6, 2)
	MARLIN_MESSAGES_UINT_FIELD(32, dst_conn_id, 2, 6)
	MARLIN_MESSAGES_UINT_FIELD(16, stream_id, 10)
	MARLIN_MESSAGES_UINT_FIELD(64, offset, 12)
};

template<typename BaseMessageType>
struct FLUSHSTREAMWrapper {
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

	FLUSHSTREAMWrapper(BaseMessageType&& base) : base(std::move(base)) {}

	using SelfType = FLUSHSTREAMWrapper<BaseMessageType>;

	MARLIN_MESSAGES_UINT_FIELD(32, src_conn_id, 6, 2)
	MARLIN_MESSAGES_UINT_FIELD(32, dst_conn_id, 2, 6)
	MARLIN_MESSAGES_UINT_FIELD(16, stream_id, 10)
	MARLIN_MESSAGES_UINT_FIELD(64, offset, 12)
};

template<typename BaseMessageType>
struct FLUSHCONFWrapper {
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

	FLUSHCONFWrapper(BaseMessageType&& base) : base(std::move(base)) {}

	using SelfType = FLUSHCONFWrapper<BaseMessageType>;

	MARLIN_MESSAGES_UINT_FIELD(32, src_conn_id, 6, 2)
	MARLIN_MESSAGES_UINT_FIELD(32, dst_conn_id, 2, 6)
	MARLIN_MESSAGES_UINT_FIELD(16, stream_id, 10)
};

#undef MARLIN_MESSAGES_PAYLOAD_FIELD
#undef MARLIN_MESSAGES_UINT_FIELD
#undef MARLIN_MESSAGES_GET_UINT_FIELD
#undef MARLIN_MESSAGES_GET_4
#undef MARLIN_MESSAGES_UINT_FIELD_SINGLE_OFFSET
#undef MARLIN_MESSAGES_UINT_FIELD_MULTIPLE_OFFSET

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_MESSAGES_HPP
