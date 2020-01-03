#ifndef POWBLOCK_H_
#define POWBLOCK_H_

#include "./Block.h"

class PoWBlock : public Block {
private:
	long difficulty;

public:
	PoWBlock(int _parentBlockId, int _blockProducerId, long _tickStamp); 
	PoWBlock getGenesisBlock();
};

#endif /*POWBLOCK_H_*/
