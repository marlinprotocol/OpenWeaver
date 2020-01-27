#ifndef MINER_H_
#define MINER_H_

#include <random>
#include <set>

#include "./Node.h"
#include "../../EventManagement/EventManager/EventManager.h"
#include "../../../helpers/Logger/easylogging.h"

class Miner : public Node {
private:
	std::mt19937_64 rng;
	int miningEventId; // default: -1 = not mining
	long long hashpower;
	std::set<int> receivedBlocks;
	std::exponential_distribution<double> exp;

public:
	Miner(int _nodeId, bool _isAlive, int _region, 
		  std::unique_ptr<BlockchainManagementModel> _blockchainManagementModel,
		  std::shared_ptr<BlockCache> _blockCache);
	uint64_t getRandomNumFromExponentialDistribtuion();
	uint64_t getTicksUntilNextBlock();
	void onNewBlockIdMessage(std::shared_ptr<NewBlockIdMessage> _message, EventManager* _eventManager);
	void onNewBlockMinedMessage(std::shared_ptr<NewBlockMinedMessage> _message, EventManager* _eventManager, uint64_t _currentTick);
};

#endif /*MINER_H_*/