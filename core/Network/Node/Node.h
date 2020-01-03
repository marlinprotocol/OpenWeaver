#ifndef NODE_H_
#define NODE_H_

#include <vector>

#include "../../Blockchain/Blockchain.h"

class Node {
private:
	int nodeId;
	bool isAlive;
	int region;
	std::vector<long long> latencyToOtherNodes;
	Blockchain blockchain;

public:
	Node(int _nodeId, bool _isAlive, int _region, 
		 std::unique_ptr<BlockchainManagementModel> _blockchainManagementModel,
		 std::shared_ptr<BlockCache> _blockCache);
	int getRegion() const;
	int getNodeId() const;
};

#endif /*NODE_H_*/
