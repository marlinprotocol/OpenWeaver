#ifndef NODE_H_
#define NODE_H_

#include <memory>
#include <vector>

#include "../Messages/Message.h"
#include "../Messages/NewBlockIdMessage.h"
#include "../Messages/NewBlockMinedMessage.h"
#include "../../Blockchain/Blockchain.h"
#include "../../../helpers/Logger/easylogging.h"

class EventManager;

class Node {
private:
	int nodeId;
	bool isAlive;
	int region;
	std::vector<long long> latencyToOtherNodes;

protected:	
	Blockchain blockchain;

public:
	Node(int _nodeId, bool _isAlive, int _region, 
		 std::unique_ptr<BlockchainManagementModel> _blockchainManagementModel,
		 std::shared_ptr<BlockCache> _blockCache);
	int getRegion() const;
	int getNodeId() const;
	virtual void onNewBlockIdMessage(std::shared_ptr<NewBlockIdMessage> _message, EventManager* _eventManager) = 0;
	virtual void onNewBlockMinedMessage(std::shared_ptr<NewBlockMinedMessage> _message, EventManager* _eventManager, uint64_t _currentTick) = 0;
};

#endif /*NODE_H_*/
