#ifndef MINER_H_
#define MINER_H_

#include <set>

#include "./Node.h"
#include "../../EventManagement/EventManager/EventManager.h"
#include "../../../helpers/Logger/easylogging.h"

class Miner : public Node {
private:
	int miningEventId; // default: -1 = not mining
	long long hashpower;
	std::set<int> receivedBlocks;

public:
	Miner(int _nodeId, bool _isAlive, int _region, 
		  std::unique_ptr<BlockchainManagementModel> _blockchainManagementModel,
		  std::shared_ptr<BlockCache> _blockCache);
	void onNewBlockIdMessage(std::shared_ptr<NewBlockIdMessage> _message, EventManager* _eventManager);
};

#endif /*MINER_H_*/