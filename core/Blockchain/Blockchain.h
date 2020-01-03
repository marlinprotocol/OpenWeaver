#ifndef BLOCKCHAIN_H_
#define BLOCKCHAIN_H_

#include <memory>
#include <vector>

#include "./Block/PoWBlock.h"
#include "./Cache/BlockCache.h"
#include "../../helpers/CircularArray.h"

class Blockchain {
private:
	Queue<std::vector<int>> blockchain;
	std::shared_ptr<BlockCache> blockCache;
	bool addBlockAtHeight(int blockId, int blockHeight);

public:
	Blockchain(std::shared_ptr<BlockCache> _blockCache);
	int getBlockchainHeight();
	int oldestBlockHeightInCache();
	int getBlockchainStorageSize();
	bool hasBlock(int blockHeight, int blockId);
	bool addBlock(PoWBlock block);
};

#endif /*BLOCKCHAIN_H_*/
