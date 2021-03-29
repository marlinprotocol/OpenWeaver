#ifndef MARLIN_UTILS_DROP_RANGES_HPP
#define MARLIN_UTILS_DROP_RANGES_HPP

#include <unordered_map>
#include <set>
#include <spdlog/spdlog.h>
#include <absl/container/flat_hash_map.h>

namespace marlin {
namespace utils {

class PacketDropRanges {

private:
	/// range id
	uint64_t range_id = 1;
        /// drop lower limit
        uint64_t lower_limit = 0;
	/// drop higher limit
	uint64_t upper_limit = 0;
	/// drop interval
	uint64_t drop_interval = 0;

	bool valid_bounds(uint64_t low, uint64_t high, uint64_t drop_gap[[maybe_unused]]){
		if( low>high )
		       return false;	
		return true;
	}

	void change_ranges(){
		ranges.clear();
                for( uint64_t i = lower_limit; i <= upper_limit; i+=drop_interval) {
                        ranges.try_emplace(i,range_id);
                }
	};

public:
	absl::flat_hash_map<uint64_t, uint64_t> ranges;

	PacketDropRanges(uint64_t low, uint64_t high, uint64_t drop_gap) {
		lower_limit = low;
		upper_limit = high;
		drop_interval = drop_gap;
		for( uint64_t i = lower_limit; i <= upper_limit; i+=drop_interval) {
			ranges.try_emplace(i,range_id);
		}
	}

	void change_lower_limit(uint64_t low) {
		lower_limit = low;
		change_ranges();
	}

	void change_upper_limit(uint64_t high) {
		upper_limit = high;
		change_ranges();
	}

        void change_drop_interval(uint64_t drop_gap) {
		drop_interval = drop_gap;
		change_ranges();
        }

	void extend_drop_ranges(uint64_t low, uint64_t high, uint64_t drop_gap) {

		range_id++;

		for(uint64_t i=low; i<= high; i+= drop_gap) {
			ranges.try_emplace(i, range_id);
		}

		lower_limit = lower_limit<low?low:lower_limit;
		upper_limit = upper_limit<high?high:upper_limit;
		return;
	}

};

} // namespace utils
} // namespace marlin

#endif // MARLIN_UTILS_DROP_RANGES_HPP

