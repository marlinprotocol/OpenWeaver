#include "./NewBlockMinedMessage.h"
#include "../../Blockchain/Block/Block.h"
#include "../../Blockchain/Block/PoWBlock.h"

NewBlockMinedMessage::NewBlockMinedMessage(int _parentBlockId) : Message(MessageType::NEW_BLOCK_MINED), parentBlockId(_parentBlockId) {
	
}

int NewBlockMinedMessage::createNewBlockObject(std::shared_ptr<BlockCache> _blockCache, int _blockProducerId, uint64_t _tickStamp) {
	int parentBlockHeight = _blockCache->getBlockById(parentBlockId)->getBlockHeight();
	newBlock = std::shared_ptr<Block>(new PoWBlock(parentBlockId, _blockProducerId, parentBlockHeight + 1, _tickStamp)); 
	_blockCache->insert(newBlock);

	return newBlock->getBlockId();
}

int NewBlockMinedMessage::getBlockId() {
	return newBlock->getBlockId();
}

std::string NewBlockMinedMessage::getType() {
	return "NewBlockMinedMessage";
}