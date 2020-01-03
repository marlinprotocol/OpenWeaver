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
	// Blockchain blockchain;

public:
	Node(int _nodeId, bool _isAlive, int _region);
	int getRegion() const;
};

#endif /*NODE_H_*/
