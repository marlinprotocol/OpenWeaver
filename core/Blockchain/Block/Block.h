#ifndef BLOCK_H_
#define BLOCK_H_

// #include "./Node.h"

class Node;

class Block {
private:
	static int blockId;
	int blockHeight;
	Block* parentBlock;
	Node* blockProducer;
	long tickStamp;

public:
	Block(Block* _parentBlock, Node* _blockProducer, long _tickStamp);
	int getBlockId();
	int getBlockHeight();
	Block* getParentBlock();
	Node* getBlockProducer();
	long getTickStamp();
	Block getGenesisBlock();
};

#endif /*BLOCK_H_*/
