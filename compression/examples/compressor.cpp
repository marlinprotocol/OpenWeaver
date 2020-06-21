#include <marlin/compression/BlockCompressor.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

using namespace marlin::core;
using namespace marlin::compression;


int main() {
	BlockCompressor c;

	c.add_txn(Buffer({1,1,1,1,1}, 5), 0);
	c.add_txn(Buffer({2,2,2,2,2}, 5), 1);
	c.add_txn(Buffer({3,3,3,3,3}, 5), 2);
	c.add_txn(Buffer({4,4,4,4,4}, 5), 3);

	return 0;
}
