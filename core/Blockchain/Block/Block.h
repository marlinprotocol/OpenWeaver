#ifndef BLOCK_H_
#define BLOCK_H_

// #include "./Node.h"
#include "../../../helpers/Logger/easylogging.h"

class Node;

class Block {
private:
	static int count;
	int id;
	int blockHeight;
	int parentBlockId;
	int blockProducerId;
	long tickStamp;

public:
	Block(int _parentBlockId, int _blockProducerId, int _blockHeight);
	Block(int _parentBlockId, int _blockProducerId, int _blockHeight, long _tickStamp);
	int getBlockId();
	int getBlockHeight();
	int getParentBlockId();
	int getBlockProducerId();
	long getTickStamp();
};

#endif /*BLOCK_H_*/
