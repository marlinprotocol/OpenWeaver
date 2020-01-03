#include "./BlockCache.h"

bool BlockCache::insert(int blockId, std::shared_ptr<Block> blockPtr) {
	blockIdBodyMap[blockId] = blockPtr;
	return true;
}

std::shared_ptr<Block> BlockCache::getBlockById(int blockId) {
	return blockIdBodyMap[blockId];
}

bool BlockCache::hasBlockId(int blockId) {
	return blockIdBodyMap.find(blockId) != blockIdBodyMap.end();
}