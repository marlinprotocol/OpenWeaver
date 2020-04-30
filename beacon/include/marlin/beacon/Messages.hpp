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
struct DISCPROTO : public core::Buffer {
public:
	DISCPROTO() : core::Buffer({0, 0}, 2) {}
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
struct LISTPROTO : public core::Buffer {
public:
	template<typename Container = std::vector<std::tuple<uint32_t, uint16_t, uint16_t>>>
	LISTPROTO(Container const& protocols = {}) : core::Buffer(
		{0, 1, static_cast<uint8_t>(protocols.size())},
		3 + protocols.size()*8
	) {
		auto iter = protocols.begin();
		for(
			auto i = 3;
			iter != protocols.end();
			iter++, i += 8
		) {
			auto [protocol, version, port] = *iter;

			this->write_uint32_be_unsafe(i, protocol);
			this->write_uint16_be_unsafe(i+4, version);
			this->write_uint16_be_unsafe(i+6, port);
		}
	}

	LISTPROTO(core::Buffer&& buf) : core::Buffer(std::move(buf)) {}

	[[nodiscard]] bool validate() const {
		if(this->size() < 3) {
			return false;
		}

		uint8_t num_proto = this->read_uint8_unsafe(2);

		// Bounds check
		if(this->size() < 3 + num_proto*8) {
			return false;
		}

		return true;
	}

	struct iterator {
	private:
		core::Buffer const* buf = nullptr;
		size_t offset = 0;
	public:
		// For iterator_traits
		using difference_type = int32_t;
		using value_type = std::tuple<uint32_t, uint16_t, uint16_t>;
		using pointer = value_type const*;
		using reference = value_type const&;
		using iterator_category = std::input_iterator_tag;

		iterator(core::Buffer const* buf, size_t offset = 0) : buf(buf), offset(offset) {}

		value_type operator*() const {
			uint32_t protocol = buf->read_uint32_be_unsafe(offset);
			uint16_t version = buf->read_uint16_be_unsafe(4 + offset);
			uint16_t port = buf->read_uint16_be_unsafe(6 + offset);

			return std::make_tuple(protocol, version, port);
		}

		iterator& operator++() {
			offset += 8;

			return *this;
		}

		bool operator==(iterator const& other) const {
			return buf == other.buf && offset == other.offset;
		}

		bool operator!=(iterator const& other) const {
			return !(*this == other);
		}
	};

	iterator cbegin() const {
		return iterator(this, 3);
	}

	iterator cend() const {
		uint8_t num_proto = this->read_uint8_unsafe(2);
		return iterator(this, 3 + num_proto*8);
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
struct DISCPEER : public core::Buffer {
public:
	DISCPEER() : core::Buffer({0, 2}, 2) {}
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
struct LISTPEER : public core::Buffer {
public:
	template<typename It = std::pair<core::SocketAddress, std::array<uint8_t, 32>>*>
	LISTPEER(It& begin = nullptr, It end = nullptr) : core::Buffer(
		{0, 1},
		1400
	) {
		size_t idx = 2;
		while(begin != end && idx + 8 + crypto_box_PUBLICKEYBYTES <= 1400) {
			begin->first.serialize(this->data()+idx, 8);
			this->write_unsafe(idx+8, begin->second.data(), crypto_box_PUBLICKEYBYTES);
			idx += 8 + crypto_box_PUBLICKEYBYTES;

			++begin;
		}

		this->truncate_unsafe(1400-idx);
	}
};

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_MESSAGES_HPP
