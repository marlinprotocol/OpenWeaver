#ifndef BLOCKCHAINMGMTMODEL_H_
#define BLOCKCHAINMGMTMODEL_H_

#include "../../helpers/Logger/easylogging.h"

class BlockchainManagementModel {
public:
	virtual ~BlockchainManagementModel() {}
	virtual void OnOutOfRangeNewBlockArrival() = 0;
};

#endif /*BLOCKCHAINMGMTMODEL_H_*/
