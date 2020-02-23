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
		  std::shared_ptr<BlockchainManagementModel> _blockchainManagementModel,
		  std::shared_ptr<BlockCache> _blockCache);
	long long getHashPower();
	void onNewBlockIdMessage(std::shared_ptr<NewBlockIdMessage> _message, EventManager* _eventManager);
	void onNewBlockMinedMessage(std::shared_ptr<NewBlockMinedMessage> _message, EventManager* _eventManager, uint64_t _currentTick);
};

#endif /*MINER_H_*/