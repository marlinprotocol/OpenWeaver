#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include <memory>

#include "../Blockchain/Cache/BlockCache.h"
#include "../EventManagement/EventManager/EventManager.h"
#include "../../models/BlockchainManagementModels/BitcoinModel.h"
#include "../../models/BlockchainManagementModels/BlockchainManagementModel.h"
#include "../Network/Network.h"
#include "../../helpers/Logger/easylogging.h"

class Simulator {
private:
	std::shared_ptr<BlockCache> blockCache;
	std::shared_ptr<BlockchainManagementModel> blockchainManagementModel;
	EventManager eventManager;
	Network network;

	void sendGenesisBlockToAllNodes(const Network& network, int genesisBlockId);

public:
	Simulator();
	bool setup();
	void start();
};

#endif /*SIMULATOR_H_*/
