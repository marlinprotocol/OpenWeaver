#ifndef POSBLOCK_H_
#define POSBLOCK_H_

#include "./Block.h"
#include "../../../helpers/Logger/easylogging.h"

class PoSBlock : public Block {
private:
	

public:
	PoSBlock(int _parentBlockId, int _blockProducerId, long _tickStamp); 
};

#endif /*POSBLOCK_H_*/
