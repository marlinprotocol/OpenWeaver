#ifndef MARLIN_COMPRESSION_BLOCKCOMPRESSOR_HPP
#define MARLIN_COMPRESSION_BLOCKCOMPRESSOR_HPP

#include <marlin/core/Buffer.hpp>
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
	void add_txn(core::Buffer&& txn, uint64_t timestamp) {
		CryptoPP::BLAKE2b blake2b((uint)8);
		blake2b.Update(txn.data(), txn.size());
		uint64_t txn_id;
		blake2b.TruncatedFinal((uint8_t*)&txn_id, 8);

		txns.try_emplace(
			txn_id,
			std::move(txn)
		);

		txn_seen[txn_id] = timestamp;
	}

	void remove_txn(core::WeakBuffer const& txn) {
		CryptoPP::BLAKE2b blake2b((uint)8);
		blake2b.Update(txn.data(), txn.size());
		uint64_t txn_id;
		blake2b.TruncatedFinal((uint8_t*)&txn_id, 8);

		txns.erase(txn_id);
		txn_seen.erase(txn_id);
	}

	void remove_txn(uint64_t txn_id) {
		txns.erase(txn_id);
		txn_seen.erase(txn_id);
	}

	core::Buffer compress(
		std::vector<core::WeakBuffer> const& misc_bufs,
		std::vector<core::WeakBuffer> const& txn_bufs
	) const {
		// Compute total size of misc bufs
		size_t misc_size = std::accumulate(
			misc_bufs.begin(),
			misc_bufs.end(),
			0,
			[](size_t size, core::WeakBuffer const& buf) { return size + buf.size(); }
		);
		size_t total_size = misc_size + txn_bufs.size()*8 + 10000; // Add space for txn ids and extra space for any full size txns
		core::Buffer final_buf(total_size);
		size_t offset = 0;

		// Copy misc bufs
		final_buf.write_uint64_le_unsafe(offset, misc_size + 8*misc_bufs.size());
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
				final_buf.write_uint64_unsafe(offset+1, txn_id);
				// Note: Write txn_id without endian conversions,
				// the hash was directly copied to txn_id memory

				offset += 9;
			}
		}

		final_buf.truncate_unsafe(final_buf.size() - offset);

		return final_buf;
	}

	std::optional<std::tuple<
		std::vector<core::WeakBuffer>,  // Misc bufs
		std::vector<core::WeakBuffer>,  // Txn bufs
		std::unordered_map<uint64_t, uint64_t>  // Holes - txn_id -> idx map
	>> decompress(core::WeakBuffer const& buf) const {
		std::vector<core::WeakBuffer> misc_bufs, txn_bufs;
		std::unordered_map<uint64_t, uint64_t> holes;

		// Bounds check
		if(buf.size() < 8) return std::nullopt;
		// Read misc size
		auto misc_size = buf.read_uint64_le_unsafe(0);
		// Bounds check
		if(buf.size() < 8 + misc_size) return std::nullopt;

		size_t offset = 8;
		// Read misc items
		while(offset < 8 + misc_size) {
			// Bounds check
			if(buf.size() < offset + 8) return std::nullopt;
			// Read misc item size
			auto item_size = buf.read_uint64_le_unsafe(offset);
			// Bounds check
			if(buf.size() < offset + 8 + item_size) return std::nullopt;
			// Add misc item
			// FIXME: Const stripping, vector cannot hold const objects
			misc_bufs.emplace_back((uint8_t*)buf.data() + offset + 8, item_size);

			offset += 8 + item_size;
		}

		// Malformed block
		if(offset != 8 + misc_size) return std::nullopt;

		// Read txns
		while(offset < buf.size()) {
			// Bounds check
			if(buf.size() < offset + 9) return std::nullopt;

			// Check type of txn encoding
			uint8_t type = buf.read_uint8_unsafe(offset);
			if(type == 0x00) { // Full txn
				// Get txn size
				auto txn_size = buf.read_uint64_le_unsafe(offset + 1);
				// Bounds check
				if(buf.size() < offset + 9 + txn_size) return std::nullopt;

				// Add txn
				// FIXME: Const stripping, vector cannot hold const objects
				txn_bufs.emplace_back((uint8_t*)buf.data() + offset + 9, txn_size);

				offset += 9 + txn_size;
			} else if(type == 0x01) { // Txn id
				// Get txn id
				auto txn_id = buf.read_uint64_unsafe(offset + 1);
				// Note: Read txn_id without endian conversions,
				// the hash was directly copied to txn_id memory

				// Find txn
				auto iter = txns.find(txn_id);
				if(iter == txns.end()) {
					// Add hole
					holes[txn_id] = txn_bufs.size();
					// Add empty marker
					txn_bufs.emplace_back(nullptr, 0);
				} else {
					// Add txn
					// FIXME: Const stripping, vector cannot hold const objects
					txn_bufs.emplace_back((uint8_t*)iter->second.data(), iter->second.size());
				}

				offset += 9;
			} else {
				return std::nullopt;
			}
		}

		return std::make_tuple(std::move(misc_bufs), std::move(txn_bufs), std::move(holes));
	}
};

} // namespace compression
} // namespace marlin

#endif // MARLIN_COMPRESSION_BLOCKCOMPRESSOR_HPP
