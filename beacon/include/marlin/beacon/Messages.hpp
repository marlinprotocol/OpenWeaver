#ifndef MARLIN_BEACON_MESSAGES_HPP
#define MARLIN_BEACON_MESSAGES_HPP

#include <marlin/core/Buffer.hpp>


namespace marlin {
namespace beacon {

/*!
\verbatim

0               1               2
0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0
+++++++++++++++++++++++++++++++++
|      0x00     |      0x00     |
+++++++++++++++++++++++++++++++++

\endverbatim
*/
template<typename BaseMessageType>
struct DISCPROTOWrapper {
	BaseMessageType base;

	DISCPROTOWrapper() : base(1) {
		base.set_payload({0});
	}

	operator BaseMessageType() && {
		return std::move(base);
	}
};

/*!
\verbatim

0               1               2               3
0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+++++++++++++++++++++++++++++++++++++++++++++++++
|      0x00     |      0x01     |Num entries (N)|
-----------------------------------------------------------------
|                      Protocol Number (1)                      |
-----------------------------------------------------------------
|          Version (1)          |            Port (1)           |
-----------------------------------------------------------------
|                      Protocol Number (2)                      |
-----------------------------------------------------------------
|          Version (2)          |            Port (2)           |
-----------------------------------------------------------------
|                              ...                              |
-----------------------------------------------------------------
|                      Protocol Number (N)                      |
-----------------------------------------------------------------
|          Version (N)          |            Port (N)           |
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

\endverbatim
*/
template<typename BaseMessageType>
struct LISTPROTOWrapper {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	LISTPROTOWrapper(size_t num_proto = 0) : base(2+8*num_proto) {
		base.set_payload({1});
	}

	LISTPROTOWrapper(BaseMessageType&& base) : base(std::move(base)) {}

	template<typename It>
	LISTPROTOWrapper& set_protocols(It begin, It end) & {
		size_t count = 0;
		while(begin != end) {
			auto [protocol, version, port] = *begin;

			auto i = 2 + count*8;
			base.payload_buffer().write_uint32_le_unsafe(i, protocol);
			base.payload_buffer().write_uint16_le_unsafe(i+4, version);
			base.payload_buffer().write_uint16_le_unsafe(i+6, port);
			count++;

			++begin;
		}
		base.payload_buffer().write_uint8_unsafe(1, count);
		base.truncate_unsafe(base.payload_buffer().size() - (2 + count*8));

		return *this;
	}

	template<typename It>
	LISTPROTOWrapper&& set_protocols(It begin, It end) && {
		return std::move(set_protocols(begin, end));
	}

	[[nodiscard]] bool validate() const {
		if(base.payload_buffer().size() < 2) {
			return false;
		}

		uint8_t num_proto = base.payload_buffer().read_uint8_unsafe(1);

		// Bounds check
		if(base.payload_buffer().size() < 2 + (size_t)num_proto*8) {
			return false;
		}

		return true;
	}

	struct iterator {
	private:
		core::WeakBuffer buf;
		size_t offset = 0;
	public:
		// For iterator_traits
		using difference_type = int32_t;
		using value_type = std::tuple<uint32_t, uint16_t, uint16_t>;
		using pointer = value_type const*;
		using reference = value_type const&;
		using iterator_category = std::input_iterator_tag;

		iterator(core::WeakBuffer buf, size_t offset = 0) : buf(buf), offset(offset) {}

		value_type operator*() const {
			uint32_t protocol = buf.read_uint32_le_unsafe(offset);
			uint16_t version = buf.read_uint16_le_unsafe(4 + offset);
			uint16_t port = buf.read_uint16_le_unsafe(6 + offset);

			return std::make_tuple(protocol, version, port);
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

	iterator protocols_begin() const {
		return iterator(base.payload_buffer(), 2);
	}

	iterator protocols_end() const {
		uint8_t num_proto = base.payload_buffer().read_uint8_unsafe(1);
		return iterator(base.payload_buffer(), 2 + num_proto*8);
	}
};

/*!
\verbatim

0               1               2
0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0
+++++++++++++++++++++++++++++++++
|      0x00     |      0x02     |
+++++++++++++++++++++++++++++++++

\endverbatim
*/
template<typename BaseMessageType>
struct DISCPEERWrapper {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	DISCPEERWrapper() : base(1) {
		base.set_payload({2});
	}
};

/*!
\verbatim

0               1               2               3
0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
+++++++++++++++++++++++++++++++++
|      0x00     |      0x03     |
---------------------------------
|            AF_INET            |
-----------------------------------------------------------------
|                        IPv4 Address (1)                       |
-----------------------------------------------------------------
|            Port (1)           |
---------------------------------
|            AF_INET            |
-----------------------------------------------------------------
|                        IPv4 Address (2)                       |
-----------------------------------------------------------------
|            Port (2)           |
-----------------------------------------------------------------
|                              ...                              |
-----------------------------------------------------------------
|            AF_INET            |
-----------------------------------------------------------------
|                        IPv4 Address (N)                       |
-----------------------------------------------------------------
|            Port (N)           |
+++++++++++++++++++++++++++++++++

\endverbatim
*/
template<typename BaseMessageType>
struct LISTPEERWrapper {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	LISTPEERWrapper(size_t num_peer = 0) : base(1+8*num_peer) {
		base.set_payload({3});
	}

	LISTPEERWrapper(BaseMessageType&& base) : base(std::move(base)) {}

	template<typename It>
	LISTPEERWrapper& set_peers(It& begin, It end) & {
		size_t idx = 1;
		while(begin != end && idx + 8 + crypto_box_PUBLICKEYBYTES <= base.payload_buffer().size()) {
			begin->first.serialize(base.payload()+idx, 8);
			base.payload_buffer().write_unsafe(idx+8, begin->second.data(), crypto_box_PUBLICKEYBYTES);
			idx += 8 + crypto_box_PUBLICKEYBYTES;

			++begin;
		}
		base.truncate_unsafe(base.payload_buffer().size() - idx);

		return *this;
	}

	template<typename It>
	LISTPEERWrapper&& set_peers(It& begin, It end) && {
		return std::move(set_peers(begin, end));
	}

	[[nodiscard]] bool validate() const {
		if(base.payload_buffer().size() < 1 || base.payload_buffer().size() % 8 != 1) {
			return false;
		}
		return true;
	}

	struct iterator {
	private:
		core::WeakBuffer buf;
		size_t offset = 0;
	public:
		// For iterator_traits
		using difference_type = int32_t;
		using value_type = std::tuple<core::SocketAddress, std::array<uint8_t, 32>>;
		using pointer = value_type const*;
		using reference = value_type const&;
		using iterator_category = std::input_iterator_tag;

		iterator(core::WeakBuffer buf, size_t offset = 0) : buf(buf), offset(offset) {}

		value_type operator*() const {
			auto peer_addr = core::SocketAddress::deserialize(buf.data()+offset, 8);
			std::array<uint8_t, 32> key;
			buf.read_unsafe(offset+8, key.data(), crypto_box_PUBLICKEYBYTES);

			return std::make_tuple(peer_addr, key);
		}

		iterator& operator++() {
			offset += 8 + crypto_box_PUBLICKEYBYTES;

			return *this;
		}

		bool operator==(iterator const& other) const {
			return offset == other.offset;
		}

		bool operator!=(iterator const& other) const {
			return !(*this == other);
		}
	};

	iterator peers_begin() const {
		return iterator(base.payload_buffer(), 1);
	}

	iterator peers_end() const {
		// Relies on validation ensuring correct size i.e. size % 8 = 1
		// Otherwise, need to modify size below
		return iterator(base.payload_buffer(), base.payload_buffer().size());
	}
};

/*!
\verbatim

0               1               2
0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0
+++++++++++++++++++++++++++++++++
|      0x00     |      0x04     |
+++++++++++++++++++++++++++++++++

\endverbatim
*/
template<typename BaseMessageType>
struct HEARTBEATWrapper {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	HEARTBEATWrapper() : base(1+crypto_box_PUBLICKEYBYTES) {
		base.set_payload({4});
	}

	HEARTBEATWrapper(BaseMessageType&& base) : base(std::move(base)) {}

	[[nodiscard]] bool validate() const {
		return base.payload_buffer().size() >= 1+crypto_box_PUBLICKEYBYTES;
	}

	std::array<uint8_t, 32> key_array() {
		std::array<uint8_t, 32> key;
		base.payload_buffer().read_unsafe(1, key.data(), crypto_box_PUBLICKEYBYTES);

		return key;
	}

	uint8_t* key() {
		return base.payload_buffer().data()+1;
	}

	HEARTBEATWrapper& set_key(uint8_t const* key) & {
		base.payload_buffer().write_unsafe(1, key, crypto_box_PUBLICKEYBYTES);

		return *this;
	}

	HEARTBEATWrapper&& set_key(uint8_t const* key) && {
		return std::move(set_key(key));
	}
};

template<typename BaseMessageType>
struct DISCCLUSTERWrapper {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	DISCCLUSTERWrapper() : base(1) {
		base.set_payload({9});
	}
};

template<typename BaseMessageType>
struct LISTCLUSTERWrapper {
	BaseMessageType base;

	operator BaseMessageType() && {
		return std::move(base);
	}

	LISTCLUSTERWrapper(size_t num_peer = 0) : base(1+8*num_peer) {
		base.set_payload({8});
	}

	LISTCLUSTERWrapper(BaseMessageType&& base) : base(std::move(base)) {}

	template<typename It>
	LISTCLUSTERWrapper& set_clusters(It& begin, It end) & {
		size_t idx = 1;
		while(begin != end && idx + 8 <= base.payload_buffer().size()) {
			begin->serialize(base.payload()+idx, 8);
			idx += 8;

			++begin;
		}
		base.truncate_unsafe(base.payload_buffer().size() - idx);

		return *this;
	}

	template<typename It>
	LISTCLUSTERWrapper&& set_clusters(It& begin, It end) && {
		return std::move(set_clusters(begin, end));
	}

	[[nodiscard]] bool validate() const {
		if(base.payload_buffer().size() < 1 || base.payload_buffer().size() % 8 != 1) {
			return false;
		}
		return true;
	}

	struct iterator {
	private:
		core::WeakBuffer buf;
		size_t offset = 0;
	public:
		// For iterator_traits
		using difference_type = int32_t;
		using value_type = core::SocketAddress;
		using pointer = value_type const*;
		using reference = value_type const&;
		using iterator_category = std::input_iterator_tag;

		iterator(core::WeakBuffer buf, size_t offset = 0) : buf(buf), offset(offset) {}

		value_type operator*() const {
			auto peer_addr = core::SocketAddress::deserialize(buf.data()+offset, 8);

			return peer_addr;
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

	iterator clusters_begin() const {
		return iterator(base.payload_buffer(), 1);
	}

	iterator clusters_end() const {
		// Relies on validation ensuring correct size i.e. size % 8 = 1
		// Otherwise, need to modify size below
		return iterator(base.payload_buffer(), base.payload_buffer().size());
	}
};

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_MESSAGES_HPP
