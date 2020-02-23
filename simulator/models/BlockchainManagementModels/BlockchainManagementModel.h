#ifndef BLOCKCHAINMGMTMODEL_H_
#define BLOCKCHAINMGMTMODEL_H_

#include "../../core/Blockchain/Block/Block.h"
#include "../../helpers/Logger/easylogging.h"

class BlockchainManagementModel {
public:
	virtual ~BlockchainManagementModel() {}
	virtual void OnOutOfRangeNewBlockArrival() = 0;
	virtual std::shared_ptr<Block> createGenesisBlock() = 0;
};

#endif /*BLOCKCHAINMGMTMODEL_H_*/
