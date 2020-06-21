#ifndef MARLIN_COMPRESSION_BLOCKCOMPRESSOR_HPP
#define MARLIN_COMPRESSION_BLOCKCOMPRESSOR_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/asyncio/core/EventLoop.hpp>
#include <cryptopp/blake2.h>

#include <unordered_map>
#include <numeric>


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

	core::Buffer compress(
		std::vector<core::WeakBuffer> const& misc_bufs,
		std::vector<core::WeakBuffer> const& txn_bufs
	) {
		// Compute total size of misc bufs
		size_t misc_size = std::transform_reduce(
			misc_bufs.begin(),
			misc_bufs.end(),
			0,
			std::plus<>(),
			[](core::WeakBuffer const& buf) { return buf.size(); }
		);
		size_t total_size = misc_size + txn_bufs.size()*8 + 10000; // Add space for txn ids and extra space for any full size txns
		core::Buffer final_buf(total_size);
		size_t offset = 0;

		// Copy misc bufs
		final_buf.write_uint64_le_unsafe(offset, misc_size);
		offset += 8;
		for(auto iter = misc_bufs.begin(); iter != misc_bufs.end(); iter++) {
			final_buf.write_uint64_le_unsafe(offset, iter->size());
			offset += 8;
			final_buf.write_unsafe(offset, iter->data(), iter->size());
			offset += iter->size();
		}

		CryptoPP::BLAKE2b blake2b((uint)8);
		for(auto iter = txn_bufs.begin(); iter != txn_bufs.end(); iter++) {
			blake2b.Update(iter->data(), iter->size());
			uint64_t txn_id;
			blake2b.TruncatedFinal((uint8_t*)&txn_id, 8);
			auto txn_iter = txns.find(txn_id);

			if(txn_iter == txns.end()) {
				// Txn not in cache, encode in full
				if(offset + txn_bufs.size()*8 > total_size)
					throw; // TODO: Handle insufficient buffer size

				final_buf.write_uint8_unsafe(offset, 0x00);
				final_buf.write_uint64_le_unsafe(offset+1, iter->size());
				final_buf.write_unsafe(offset+9, iter->data(), iter->size());

				offset += 9 + iter->size();
			} else {
				// Found txn in cache, copy id
				final_buf.write_uint8_unsafe(offset, 0x01);
				final_buf.write_uint64_le_unsafe(offset+1, txn_id);

				offset += 9;
			}
		}

		return final_buf;
	}
};

} // namespace compression
} // namespace marlin

#endif // MARLIN_COMPRESSION_BLOCKCOMPRESSOR_HPP
