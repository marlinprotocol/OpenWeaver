#ifndef MARLIN_COMPRESSION_BLOCKCOMPRESSOR_HPP
#define MARLIN_COMPRESSION_BLOCKCOMPRESSOR_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/asyncio/EventLoop.hpp>
#include <cryptopp/blake2.h>

#include <unordered_map>


namespace marlin {
namespace compression {

struct BlockCompressor {
private:
	std::unordered_map<uint64_t, core::Buffer> txns;
	std::unordered_map<uint64_t, uint64_t> txn_seen;
public:
	void add_txn(core::Buffer&& txn) {
		CryptoPP::BLAKE2b blake2b((uint)8);
		blake2b.Update(txn.data(), txn.size());
		uint64_t txn_id;
		blake2b.TruncatedFinal((uint8_t*)&txn_id, 8);

		txns.try_emplace(
			txn_id,
			std::move(txn)
		);

		txn_seen[txn_id] = asyncio::EventLoop::now();
	}
};

} // namespace compression
} // namespace marlin

#endif // MARLIN_COMPRESSION_BLOCKCOMPRESSOR_HPP
