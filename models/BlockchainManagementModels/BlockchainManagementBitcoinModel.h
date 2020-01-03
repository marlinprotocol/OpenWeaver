#ifndef BLOCKCHAINMGMTBTCMODEL_H_
#define BLOCKCHAINMGMTBTCMODEL_H_

#include "./BlockchainManagementModel.h"

class BlockchainManagementBitcoinModel : public BlockchainManagementModel {
public:
	void OnOutOfRangeNewBlockArrival();
};

#endif /*BLOCKCHAINMGMTBTCMODEL_H_*/
