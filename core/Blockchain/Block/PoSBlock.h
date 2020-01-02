#ifndef POSBLOCK_H_
#define POSBLOCK_H_

#include "./Block.h"

class PoSBlock : public Block {
private:
	

public:
	PoSBlock(Block* _parentBlock, Node* _blockProducer, long _tickStamp); 
};

#endif /*POSBLOCK_H_*/
