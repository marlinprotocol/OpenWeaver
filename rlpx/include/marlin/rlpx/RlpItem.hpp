#ifndef MARLIN_RLPX_RLPITEM_HPP
#define MARLIN_RLPX_RLPITEM_HPP

#include <vector>
#include <utility>
#include <initializer_list>
#include <numeric>

namespace marlin {
namespace rlpx {

class RlpItem {
private:
	enum struct Type {
		String,
		List
	} type;

	uint8_t *str;
	size_t len;

	std::vector<RlpItem> items;

	static uint8_t num_bytes(uint64_t len) {
		uint8_t num = 0;
		while(len > 0) {
			num += 1;
			len >>= 8;
		}
		return num;
	}
public:
	size_t enc_size = 0;

	static uint8_t uint_encode(uint64_t num, uint8_t *out) {
		uint8_t nb = num_bytes(num);
		for(int i = nb; i > 0; i++) {
			out[i - 1] = (uint8_t)num;
			num >>= 8;
		}
		return nb;
	}

	static RlpItem from_str(uint8_t *str, uint64_t len) {
		RlpItem item;
		item.type = Type::String;
		item.str = str;
		item.len = len;

		if(len == 1 && str[0] < 0x80) {
			item.enc_size = 1;
		} else if(len < 56) {
			item.enc_size = 1 + len;
		} else {
			item.enc_size = 1;
			item.enc_size += num_bytes(len);
			item.enc_size += len;
		}

		return item;
	}

	static RlpItem from_list(std::initializer_list<RlpItem> list) {
		RlpItem item;
		item.type = Type::List;

		for(auto iter = list.begin(); iter != list.end(); iter++) {
			item.items.emplace_back(*iter);
		}

		item.enc_size = std::accumulate(
			item.items.begin(),
			item.items.end(),
			0,
			[] (size_t const &acc, RlpItem const &item) {
				return acc + item.enc_size;
			}
		);
		if(item.enc_size < 56) {
			item.enc_size += 1;
		} else {
			item.enc_size += num_bytes(item.enc_size);
			item.enc_size += 1;
		}

		return item;
	}

	void encode(uint8_t *out) {
		if(type == Type::String) {
			if(len == 1 && str[0] < 0x80) {
				out[0] = str[0];
			} else if(len < 56) {
				out[0] = 0x80 + len;
				std::memcpy(out + 1, str, len);
			} else {
				out[0] = 0xb7 + num_bytes(len);
				uint_encode(len, out + 1);
				std::memcpy(out + 1 + num_bytes(len), str, len);
			}
		} else if(type == Type::List) {
			if(enc_size < 57) {
				out[0] = 0xc0 + enc_size - 1;
				out = out + 1;
				for(uint i = 0; i < items.size(); i++) {
					items[i].encode(out);
					out = out + items[i].enc_size;
				}
			} else {
				uint64_t num = std::accumulate(
					items.begin(),
					items.end(),
					0,
					[] (size_t const &acc, RlpItem const &item) {
						return acc + item.enc_size;
					}
				);
				out[0] = 0xf7 + num_bytes(num);
				uint_encode(num, out + 1);
				out = out + 1 + num_bytes(num);
				for(uint i = 0; i < items.size(); i++) {
					items[i].encode(out);
					out = out + items[i].enc_size;
				}
			}
		}
	}
};

} // namespace rlpx
} // namespace marlin

#endif // MARLIN_RLPX_RLPITEM_HPP
