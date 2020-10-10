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
#define MARLIN_MESSAGES_UINT_FIELD(...) MARLIN_MESSAGES_GET_4(__VA_ARGS__, MARLIN_MESSAGES_UINT_FIELD_MULTIPLE_OFFSET(__VA_ARGS__), MARLIN_MESSAGES_UINT_FIELD_SINGLE_OFFSET(__VA_ARGS__), throw)
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
	core::WeakBuffer payload_buffer() & { \
		return base.payload_buffer().cover_unsafe(offset); \
	} \
 \
	/** @brief Get a Buffer corresponding to the payload area, consumes the object */ \
	core::Buffer payload_buffer() && { \
		return std::move(base).payload_buffer().cover_unsafe(offset); \
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
