#include "./BlockCache.h"

bool BlockCache::insert(std::shared_ptr<Block> blockPtr) {
	int blockId = blockPtr->getBlockId();
	blockIdBodyMap[blockId] = blockPtr;
	return true;
}

std::shared_ptr<Block> BlockCache::getBlockById(int blockId) const {
	return blockIdBodyMap.find(blockId)->second;
}

bool BlockCache::hasBlockId(int blockId) {
	return blockIdBodyMap.find(blockId) != blockIdBodyMap.end();
}