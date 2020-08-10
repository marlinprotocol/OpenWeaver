#ifndef MARLIN_STREAM_ACK_RANGES_HPP
#define MARLIN_STREAM_ACK_RANGES_HPP

#include <list>
#include <spdlog/spdlog.h>

namespace marlin {
namespace stream {

/// Stores ranges of packet numbers that have and haven't been seen
class AckRanges {
public:
	/// Alternating ranges of seen and not seen packet numbers
	std::list<uint64_t> ranges;
	/// Largest seen packet number
	uint64_t largest;

	/// Mark a packet number as seen
	void add_packet_number(uint64_t num) {
		// Initial
		if(ranges.size() == 0) {
			ranges.push_back(1);

			largest = num;
			return;
		}

		// Check beginning
		if(num > largest) {
			if(num == largest + 1) {
				ranges.front()++;
			} else {
				ranges.push_front(num - largest - 1);
				ranges.push_front(1);
			}

			largest = num;
			return;
		}

		uint64_t high = largest;
		bool gap = false;
		for(auto iter = ranges.begin(); iter != ranges.end(); iter++) {
			uint64_t low = high - *iter;
			// Check if contained in range (low,high]
			if(high >= num && num >= low + 1) {
				if(!gap) {
					// Already in range, ignore
					return;
				}

				auto next_iter = std::next(iter);
				auto prev_iter = std::prev(iter);

				if(num == high) {
					// End of previous range
					(*prev_iter)++;
					(*iter)--;

					// Merge if possible
					if(*iter == 0) {
						*prev_iter += *next_iter;
						ranges.erase(next_iter);
						ranges.erase(iter);
					}
				} else if(num == low + 1) {
					// Start of next range
					(*next_iter)++;
					(*iter)--;

					// Merge if possible
					if(*iter == 0) {
						*prev_iter += *next_iter;
						ranges.erase(next_iter);
						ranges.erase(iter);
					}
				} else {
					// Middle of gap, create new range and gap
					*iter = num - (low + 1);
					ranges.insert(iter, high-num);
					ranges.insert(iter, 1);
				}

				return;
			}

			high = low;
			gap = !gap;
		}

		// At end
		if(high == num) {
			ranges.back()++;
		} else {
			ranges.push_back(high - num);
			ranges.push_back(1);
		}

		if(ranges.size() > 1001) {
			SPDLOG_ERROR("AckRange resized: {}", num);
			ranges.resize(1001);
		}
	}
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_ACK_RANGES_HPP
