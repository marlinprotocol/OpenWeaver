#include "./Node.h"

Node::Node(int _nodeId, bool _isAlive, int _region, 
		   std::shared_ptr<BlockchainManagementModel> _blockchainManagementModel,
		   std::shared_ptr<BlockCache> _blockCache) 
	 : nodeId(_nodeId), isAlive(_isAlive), region(_region), 
	 		  blockchain(std::const_pointer_cast<const BlockCache>(_blockCache)) {}

int Node::getRegion() const {
	return region;
}

int Node::getNodeId() const {
	return nodeId;
}