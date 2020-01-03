#ifndef MINER_H_
#define MINER_H_

#include <set>

#include "./Node.h"
#include "../../../helpers/Logger/easylogging.h"

class Miner : public Node {
private:
	long long hashpower;
	std::set<int> receivedBlocks;

public:
	Miner(int _nodeId, bool _isAlive, int _region, 
		  std::unique_ptr<BlockchainManagementModel> _blockchainManagementModel,
		  std::shared_ptr<BlockCache> _blockCache);
	void onNewBlockIdMessage(std::shared_ptr<NewBlockIdMessage> _message);
};

#endif /*MINER_H_*/