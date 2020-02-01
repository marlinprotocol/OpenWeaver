#include "./Block.h"

int Block::count = 0;

Block::Block(int _parentBlockId, int _blockProducerId, int _blockHeight) {
	parentBlockId = _parentBlockId;
	blockProducerId = _blockProducerId;
	blockHeight = _blockHeight;
	id = count++;
}

Block::Block(int _parentBlockId, int _blockProducerId, int _blockHeight, long _tickStamp) {
	parentBlockId = _parentBlockId;
	blockProducerId = _blockProducerId;
	blockHeight = _blockHeight;
	tickStamp = _tickStamp;
	id = count++;
}

int Block::getBlockId() {
	return id;
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

