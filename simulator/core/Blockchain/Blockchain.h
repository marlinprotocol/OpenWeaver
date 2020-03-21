#ifndef BLOCKCHAIN_H_
#define BLOCKCHAIN_H_

#include <memory>
#include <vector>

#include "./Block/Block.h"
#include "./Cache/BlockCache.h"
#include "../../helpers/CircularArray.h"
#include "../../models/BlockchainManagementModels/BlockchainManagementModel.h"
#include "../../helpers/Logger/easylogging.h"

class Blockchain {
private:
	Queue<std::vector<int>> blockchain;
	std::shared_ptr<const BlockCache> blockCache;
	bool addBlockAtHeight(int blockId, int blockHeight);
	std::unique_ptr<BlockchainManagementModel> blockchainManagementModel;

public:
	Blockchain(std::shared_ptr<const BlockCache> _blockCache);	
	int getBlockchainHeight();
	std::vector<int> getLatestBlockIds();
	int oldestBlockHeightInCache();
	int getBlockchainStorageSize();
	bool hasBlock(int blockHeight, int blockId);
	bool addBlock(int newBlockHeight, int newBlockId);
	std::vector<int> getBlockIdsAtHeight(int blockHeight);
};

#endif /*BLOCKCHAIN_H_*/
