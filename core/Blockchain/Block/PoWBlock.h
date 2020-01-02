#ifndef POWBLOCK_H_
#define POWBLOCK_H_

#include "./Block.h"

class PoWBlock : public Block {
private:
	long difficulty;

public:
	PoWBlock(Block* _parentBlock, Node* _blockProducer, long _tickStamp); 
};

#endif /*POWBLOCK_H_*/
