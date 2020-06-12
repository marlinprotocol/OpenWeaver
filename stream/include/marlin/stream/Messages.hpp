#ifndef MARLIN_STREAM_MESSAGES_HPP
#define MARLIN_STREAM_MESSAGES_HPP

#include <marlin/core/Buffer.hpp>


namespace marlin {
namespace stream {

#define MARLIN_MESSAGES_BASE(Derived) \
	/** @brief Message type */ \
	using SelfType = Derived<BaseMessageType>; \
 \
	/** @brief Base message */ \
	BaseMessageType base; \
 \
	/** @brief Conversion to base message */ \
	operator BaseMessageType() && { \
		return std::move(base); \
	} \
 \
	/** @brief Construct from base message */ \
	Derived(BaseMessageType&& base) : base(std::move(base)) {}

#define MARLIN_MESSAGES_UINT_FIELD_MULTIPLE_OFFSET(size, name, read_offset, write_offset) \
	/** @brief Set name */ \
	SelfType& set_##name(uint##size##_t name) & { \
		base.payload_buffer().write_uint##size##_le_unsafe(write_offset, name); \
 \
		return *this; \
	} \
 \
	/** @brief Set name */ \
	SelfType&& set_##name(uint##size##_t name) && { \
		return std::move(set_##name(name)); \
	} \
 \
	/** @brief Get name */ \
	uint##size##_t name() const { \
		return base.payload_buffer().read_uint##size##_le_unsafe(read_offset); \
	}

#define MARLIN_MESSAGES_UINT_FIELD_SINGLE_OFFSET(size, name, offset) MARLIN_MESSAGES_UINT_FIELD_MULTIPLE_OFFSET(size, name, offset, offset)

#define MARLIN_MESSAGES_GET_4(_0, _1, _2, _3, MACRO, ...) MACRO
#define MARLIN_MESSAGES_GET_UINT_FIELD(...) MARLIN_MESSAGES_GET_4(__VA_ARGS__, MARLIN_MESSAGES_UINT_FIELD_MULTIPLE_OFFSET(__VA_ARGS__), MARLIN_MESSAGES_UINT_FIELD_SINGLE_OFFSET(__VA_ARGS__), throw)
#define MARLIN_MESSAGES_UINT_FIELD(...) MARLIN_MESSAGES_GET_UINT_FIELD(__VA_ARGS__)
#define MARLIN_MESSAGES_UINT16_FIELD(...) MARLIN_MESSAGES_UINT_FIELD(16, __VA_ARGS__)
#define MARLIN_MESSAGES_UINT32_FIELD(...) MARLIN_MESSAGES_UINT_FIELD(32, __VA_ARGS__)
#define MARLIN_MESSAGES_UINT64_FIELD(...) MARLIN_MESSAGES_UINT_FIELD(64, __VA_ARGS__)

#define MARLIN_MESSAGES_PAYLOAD_FIELD(offset) \
	/** @brief Set payload given pointer to bytes and size */ \
	SelfType& set_payload(uint8_t const* in, size_t size) & { \
		base.payload_buffer().write_unsafe(offset, in, size); \
 \
		return *this; \
	} \
 \
	/** @brief Set payload given pointer to bytes and size */ \
	SelfType&& set_payload(uint8_t const* in, size_t size) && { \
		return std::move(set_payload(in, size)); \
	} \
 \
	/** @brief Set payload given initializer list with bytes */ \
	SelfType& set_payload(std::initializer_list<uint8_t> il) & { \
		base.payload_buffer().write_unsafe(offset, il.begin(), il.size()); \
 \
		return *this; \
	} \
 \
	/** @brief Set payload given initializer list with bytes */ \
	SelfType&& set_payload(std::initializer_list<uint8_t> il) && { \
		return std::move(set_payload(il)); \
	} \
 \
	/** @brief Get a WeakBuffer corresponding to the payload area */ \
	core::WeakBuffer payload_buffer() const& { \
		auto buf = base.payload_buffer(); \
		buf.cover_unsafe(offset); \
 \
		return buf; \
	} \
 \
	/** @brief Get a Buffer corresponding to the payload area, consumes the object */ \
	core::Buffer payload_buffer() && { \
		auto buf = std::move(base).payload_buffer(); \
		buf.cover_unsafe(offset); \
 \
		return std::move(buf); \
	} \
 \
	/** @brief Get a uint8_t* corresponding to the payload area */ \
	uint8_t* payload() { \
		return base.payload_buffer().data() + offset; \
	}

#define MARLIN_MESSAGES_ARRAY_FIELD(type, begin_offset, end_offset) \
private: \
	template<typename type> \
	struct type##_iterator { \
	private: \
		core::WeakBuffer buf; \
		size_t offset = 0; \
	public: \
		using difference_type = int32_t; \
		using value_type = typename type::value_type; \
		using pointer = value_type const*; \
		using reference = value_type const&; \
		using iterator_category = std::input_iterator_tag; \
 \
		type##_iterator(core::WeakBuffer buf, size_t offset = 0) : buf(buf), offset(offset) {} \
 \
		value_type operator*() const { \
			return type::read(buf, offset); \
		} \
 \
		type##_iterator& operator++() { \
			offset += type::size(buf, offset); \
 \
			return *this; \
		} \
 \
		bool operator==(type##_iterator const& other) const { \
			return offset == other.offset; \
		} \
 \
		bool operator!=(type##_iterator const& other) const { \
			return !(*this == other); \
		} \
	}; \
public: \
	/** @brief Begin iterator for type */ \
	type##_iterator<type> type##s_begin() const { \
		return type##_iterator<type>(base.payload_buffer(), begin_offset); \
	} \
 \
	/** @brief End iterator for type */ \
	type##_iterator<type> type##s_end() const { \
		return type##_iterator<type>(base.payload_buffer(), end_offset); \
	} \
 \
	/** @brief Set type array using given iterators */ \
	template<typename It> \
	SelfType& set_##type##s(It begin, It end) & { \
		size_t idx = begin_offset; \
		while(begin != end && idx + type::size(begin) <= base.payload_buffer().size()) { \
			type::write(base.payload_buffer(), idx, *begin); \
			idx += type::size(begin); \
			++begin; \
		} \
		base.truncate_unsafe(base.payload_buffer().size() - idx); \
 \
		return *this; \
	} \
 \
	/** @brief Set type array using given iterators */ \
	template<typename It> \
	SelfType&& set_##type##s(It begin, It end) && { \
		return std::move(set_##type##s(begin, end)); \
	}


/// DATA message template
template<typename BaseMessageType>
struct DATAWrapper {
	MARLIN_MESSAGES_BASE(DATAWrapper);
	MARLIN_MESSAGES_UINT32_FIELD(src_conn_id, 6, 2);
	MARLIN_MESSAGES_UINT32_FIELD(dst_conn_id, 2, 6);
	MARLIN_MESSAGES_UINT64_FIELD(packet_number, 10);
	MARLIN_MESSAGES_UINT16_FIELD(stream_id, 18);
	MARLIN_MESSAGES_UINT64_FIELD(offset, 20);
	MARLIN_MESSAGES_UINT16_FIELD(length, 28);
	MARLIN_MESSAGES_PAYLOAD_FIELD(30);

	/// Construct a DATA/FIN message with a given payload size
	DATAWrapper(size_t payload_size, bool is_fin) : base(30 + payload_size) {
		base.set_payload({0, static_cast<uint8_t>(is_fin)});
	}

	/// Validate the DATA/FIN message
	[[nodiscard]] bool validate(size_t payload_size) const {
		return base.payload_buffer().size() >= 30 + payload_size;
	}

	/// Check if the FIN bit is set
	bool is_fin_set() const {
		return base.payload_buffer().read_uint8_unsafe(1) == 1;
	}
};

/// ACK message template
template<typename BaseMessageType>
struct ACKWrapper {
	MARLIN_MESSAGES_BASE(ACKWrapper);
	MARLIN_MESSAGES_UINT32_FIELD(src_conn_id, 6, 2);
	MARLIN_MESSAGES_UINT32_FIELD(dst_conn_id, 2, 6);
	MARLIN_MESSAGES_UINT16_FIELD(size, 10);
	MARLIN_MESSAGES_UINT64_FIELD(packet_number, 12);

private:
	struct range {
		using value_type = uint64_t;

		static size_t size(core::WeakBuffer const&, uint64_t) {
			return 8;
		}

		template<typename It>
		static size_t size(It&&) {
			return 8;
		}

		static value_type read(core::WeakBuffer const& buf, uint64_t offset) {
			return buf.read_uint64_le_unsafe(offset);
		}

		static void write(core::WeakBuffer buf, uint64_t offset, value_type val) {
			buf.write_uint64_le_unsafe(offset, val);
		}
	};
public:
	MARLIN_MESSAGES_ARRAY_FIELD(range, 20, 20 + 8*size());

	/// Construct an ACK message to hold a given number of ack ranges
	ACKWrapper(size_t num_ranges) : base(20 + 8*num_ranges) {
		base.set_payload({0, 2});
	}

	/// Validate the ACK message
	[[nodiscard]] bool validate() const {
		if(base.payload_buffer().size() < 20 || base.payload_buffer().size() != 20 + size()*8) {
			return false;
		}
		return true;
	}
};

/// DIAL message template
template<typename BaseMessageType>
struct DIALWrapper {
	MARLIN_MESSAGES_BASE(DIALWrapper);
	MARLIN_MESSAGES_UINT32_FIELD(src_conn_id, 6, 2);
	MARLIN_MESSAGES_UINT32_FIELD(dst_conn_id, 2, 6);
	MARLIN_MESSAGES_PAYLOAD_FIELD(10);

	/// Construct a DIAL message to hold the given payload size
	DIALWrapper(size_t payload_size) : base(10 + payload_size) {
		base.set_payload({0, 3});
	}

	/// Validate the DIAL message
	[[nodiscard]] bool validate(size_t payload_size) const {
		return base.payload_buffer().size() >= 10 + payload_size;
	}
};

/// DIALCONF message template
template<typename BaseMessageType>
struct DIALCONFWrapper {
	MARLIN_MESSAGES_BASE(DIALCONFWrapper);
	MARLIN_MESSAGES_UINT32_FIELD(src_conn_id, 6, 2);
	MARLIN_MESSAGES_UINT32_FIELD(dst_conn_id, 2, 6);
	MARLIN_MESSAGES_PAYLOAD_FIELD(10)

	/// Construct a DIALCONF message to hold the given payload size
	DIALCONFWrapper(size_t payload_size) : base(10 + payload_size) {
		base.set_payload({0, 4});
	}

	/// Validate the DIALCONF message
	[[nodiscard]] bool validate(size_t payload_size) const {
		return base.payload_buffer().size() >= 10 + payload_size;
	}
};

/// CONF message template
template<typename BaseMessageType>
struct CONFWrapper {
	MARLIN_MESSAGES_BASE(CONFWrapper);
	MARLIN_MESSAGES_UINT32_FIELD(src_conn_id, 6, 2);
	MARLIN_MESSAGES_UINT32_FIELD(dst_conn_id, 2, 6);

	/// Construct a CONF message
	CONFWrapper() : base(10) {
		base.set_payload({0, 5});
	}

	/// Validate the CONF message
	[[nodiscard]] bool validate() const {
		return base.payload_buffer().size() >= 10;
	}
};

/// RST message template
template<typename BaseMessageType>
struct RSTWrapper {
	MARLIN_MESSAGES_BASE(RSTWrapper);
	MARLIN_MESSAGES_UINT32_FIELD(src_conn_id, 6, 2);
	MARLIN_MESSAGES_UINT32_FIELD(dst_conn_id, 2, 6);

	/// Construct a RST message
	RSTWrapper() : base(10) {
		base.set_payload({0, 6});
	}

	/// Validate the RST message
	[[nodiscard]] bool validate() const {
		return base.payload_buffer().size() >= 10;
	}
};

/// SKIPSTREAM message template
template<typename BaseMessageType>
struct SKIPSTREAMWrapper {
	MARLIN_MESSAGES_BASE(SKIPSTREAMWrapper);
	MARLIN_MESSAGES_UINT32_FIELD(src_conn_id, 6, 2);
	MARLIN_MESSAGES_UINT32_FIELD(dst_conn_id, 2, 6);
	MARLIN_MESSAGES_UINT16_FIELD(stream_id, 10);
	MARLIN_MESSAGES_UINT64_FIELD(offset, 12);

	/// Construct a SKIPSTREAM message
	SKIPSTREAMWrapper() : base(20) {
		base.set_payload({0, 7});
	}

	/// Validate the SKIPSTREAM message
	[[nodiscard]] bool validate() const {
		return base.payload_buffer().size() >= 20;
	}
};

/// FLUSHSTREAM message template
template<typename BaseMessageType>
struct FLUSHSTREAMWrapper {
	MARLIN_MESSAGES_BASE(FLUSHSTREAMWrapper);
	MARLIN_MESSAGES_UINT32_FIELD(src_conn_id, 6, 2);
	MARLIN_MESSAGES_UINT32_FIELD(dst_conn_id, 2, 6);
	MARLIN_MESSAGES_UINT16_FIELD(stream_id, 10);
	MARLIN_MESSAGES_UINT64_FIELD(offset, 12);

	/// Construct a FLUSHSTREAM message
	FLUSHSTREAMWrapper() : base(20) {
		base.set_payload({0, 8});
	}

	/// Validate the FLUSHSTREAM message
	[[nodiscard]] bool validate() const {
		return base.payload_buffer().size() >= 20;
	}
};

/// FLUSHCONF message template
template<typename BaseMessageType>
struct FLUSHCONFWrapper {
	MARLIN_MESSAGES_BASE(FLUSHCONFWrapper);
	MARLIN_MESSAGES_UINT32_FIELD(src_conn_id, 6, 2);
	MARLIN_MESSAGES_UINT32_FIELD(dst_conn_id, 2, 6);
	MARLIN_MESSAGES_UINT16_FIELD(stream_id, 10);

	/// Construct a FLUSHCONF message
	FLUSHCONFWrapper() : base(12) {
		base.set_payload({0, 9});
	}

	/// Validate the FLUSHCONF message
	[[nodiscard]] bool validate() const {
		return base.payload_buffer().size() >= 12;
	}
};

#undef MARLIN_MESSAGES_UINT16_FIELD
#undef MARLIN_MESSAGES_UINT32_FIELD
#undef MARLIN_MESSAGES_UINT64_FIELD
#undef MARLIN_MESSAGES_PAYLOAD_FIELD
#undef MARLIN_MESSAGES_UINT_FIELD
#undef MARLIN_MESSAGES_GET_UINT_FIELD
#undef MARLIN_MESSAGES_GET_4
#undef MARLIN_MESSAGES_UINT_FIELD_SINGLE_OFFSET
#undef MARLIN_MESSAGES_UINT_FIELD_MULTIPLE_OFFSET
#undef MARLIN_MESSAGES_BASE

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_MESSAGES_HPP
