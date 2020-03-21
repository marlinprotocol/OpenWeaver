#ifndef BLOCKCHAINMGMTMODEL_H_
#define BLOCKCHAINMGMTMODEL_H_

#include "../../helpers/Logger/easylogging.h"

class Block;
class Node;

class BlockchainManagementModel {
public:
	virtual ~BlockchainManagementModel() {}
	virtual void OnOutOfRangeNewBlockArrival() = 0;
	virtual std::shared_ptr<Block> createGenesisBlock() = 0;
	virtual uint64_t getNextBlockTime() = 0;
	virtual std::shared_ptr<Node> getNextBlockProducer() = 0;
};

#endif /*BLOCKCHAINMGMTMODEL_H_*/