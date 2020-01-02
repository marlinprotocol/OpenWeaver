#include "./Node.h"

Node::Node(int _nodeId, bool _isAlive, int _region) {
	nodeId = _nodeId;
	isAlive = _isAlive;
	region = _region;
}

int Node::getRegion() const {
	return region;
}