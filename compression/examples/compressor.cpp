#include <marlin/compression/BlockCompressor.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

using namespace marlin::core;
using namespace marlin::compression;


int main() {
	BlockCompressor c;

	c.add_txn(Buffer({0,0}, 2), 0);
	c.add_txn(Buffer({1,1,1,1,1}, 5), 1);
	c.add_txn(Buffer({2,2,2,2,2}, 5), 2);
	c.add_txn(Buffer({3,3,3,3,3}, 5), 3);
	c.add_txn(Buffer({4,4,4,4,4}, 5), 4);

	auto compressed = c.compress(
		{Buffer({0xff,0xff,0xff,0xff,0xff}, 5)},
		{Buffer({2,2,2,2,2}, 5),Buffer({1,1,1,1,1}, 5),Buffer({4,4,4,4,4}, 5)}
	);

	SPDLOG_INFO("Compressed: {}", spdlog::to_hex(compressed.data(), compressed.data() + compressed.size()));

	auto res = c.decompress(compressed);
	if(res == std::nullopt) {
		SPDLOG_INFO("Decompress failure");
	}

	auto& [misc_bufs, txn_bufs] = res.value();
	for(auto iter = misc_bufs.begin(); iter != misc_bufs.end(); iter++) {
		SPDLOG_INFO("Misc: {}", spdlog::to_hex(iter->data(), iter->data() + iter->size()));
	}
	for(auto iter = txn_bufs.begin(); iter != txn_bufs.end(); iter++) {
		SPDLOG_INFO("Txn: {}", spdlog::to_hex(iter->data(), iter->data() + iter->size()));
	}

	return 0;
}
