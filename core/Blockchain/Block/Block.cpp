#include "./Block.h"

int Block::blockId = 0;

Block::Block(Block* _parentBlock, Node* _blockProducer, long _tickStamp) {
	parentBlock = _parentBlock;
	blockProducer = _blockProducer;
	tickStamp = _tickStamp;
}

int Block::getBlockId() {
	return blockId;
}

int Block::getBlockHeight() {
	return blockHeight;
}

Block* Block::getParentBlock() {
	return parentBlock;
}

Node* Block::getBlockProducer() {
	return blockProducer;
}

long Block::getTickStamp() {
	return tickStamp;
}

Block Block::getGenesisBlock() {
	return Block(nullptr, nullptr, 0);
}