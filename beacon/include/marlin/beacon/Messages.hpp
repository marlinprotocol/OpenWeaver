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
	template<typename Container>
	LISTPROTO(Container const& protocols) : core::Buffer(
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
};

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_MESSAGES_HPP
