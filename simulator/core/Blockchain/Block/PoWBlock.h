#ifndef POWBLOCK_H_
#define POWBLOCK_H_

#include <memory>

#include "./Block.h"
#include "../../../helpers/Logger/easylogging.h"

class PoWBlock : public Block {
private:
	long difficulty;

public:
	PoWBlock(int _parentBlockId, int _blockProducerId, int _blockHeight); 
	PoWBlock(int _parentBlockId, int _blockProducerId, int _blockHeight, long _tickStamp); 
};

inline std::shared_ptr<Block> getGenesisPoWBlock() {
	return std::shared_ptr<Block>(new PoWBlock(-1, -1, 0, 0));
}

#endif /*POWBLOCK_H_*/
