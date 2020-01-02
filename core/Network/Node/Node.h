#ifndef NODE_H_
#define NODE_H_

#include <vector>

class Node {
private:
	int nodeId;
	bool isAlive;
	int region;
	std::vector<long long> latencyToOtherNodes;

public:
	Node(int _nodeId, bool _isAlive, int _region);
	int getRegion() const;
};

#endif /*NODE_H_*/
