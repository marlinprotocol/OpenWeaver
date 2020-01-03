#ifndef BLOCK_H_
#define BLOCK_H_

// #include "./Node.h"

class Node;

class Block {
private:
	static int blockId;
	int blockHeight;
	int parentBlockId;
	int blockProducerId;
	long tickStamp;

public:
	Block(int _parentBlockId, int _blockProducerId, long _tickStamp);
	int getBlockId();
	int getBlockHeight();
	int getParentBlockId();
	int getBlockProducerId();
	long getTickStamp();
};

#endif /*BLOCK_H_*/
