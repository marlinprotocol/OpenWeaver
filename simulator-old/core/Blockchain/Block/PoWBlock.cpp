#include "./PoWBlock.h"

PoWBlock::PoWBlock(int _parentBlockId, int _blockProducerId, int _blockHeight) 
		 : Block(_parentBlockId, _blockProducerId, _blockHeight) {
}

PoWBlock::PoWBlock(int _parentBlockId, int _blockProducerId, int _blockHeight, long _tickStamp) 
		 : Block(_parentBlockId, _blockProducerId, _blockHeight, _tickStamp) {
}