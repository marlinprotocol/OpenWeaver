#ifndef MARLIN_STREAM_CONNECTION_HPP
#define MARLIN_STREAM_CONNECTION_HPP

#include "SendStream.hpp"
#include "RecvStream.hpp"

namespace marlin {
namespace stream {

template<typename NodeType>
struct Connection {
	// enum State {
	// 	Open,
	// 	Closed
	// };
	// State state;

	uint16_t next_stream_id = 0;

	std::map<uint16_t, SendStream> send_streams;
	std::map<uint16_t, RecvStream<NodeType>> recv_streams;
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_CONNECTION_HPP
