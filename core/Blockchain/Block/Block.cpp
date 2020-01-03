#include "./Block.h"

int Block::blockId = 0;

Block::Block(int _parentBlockId, int _blockProducerId, long _tickStamp) {
	parentBlockId = _parentBlockId;
	blockProducerId = _blockProducerId;
	tickStamp = _tickStamp;
}

int Block::getBlockId() {
	return blockId;
}

int Block::getBlockHeight() {
	return blockHeight;
}

int Block::getParentBlockId() {
	return parentBlockId;
}

int Block::getBlockProducerId() {
	return blockProducerId;
}

long Block::getTickStamp() {
	return tickStamp;
}

