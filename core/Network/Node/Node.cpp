#include "./Node.h"

Node::Node(int _nodeId, bool _isAlive, int _region, 
		   std::unique_ptr<BlockchainManagementModel> _blockchainManagementModel,
		   std::shared_ptr<BlockCache> _blockCache) 
	 : nodeId(_nodeId), isAlive(_isAlive), region(_region), blockchain(_blockCache) {}

int Node::getRegion() const {
	return region;
}