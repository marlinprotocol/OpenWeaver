#include "./Miner.h"

Miner::Miner(int _nodeId, bool _isAlive, int _region, 
			 std::unique_ptr<BlockchainManagementModel> _blockchainManagementModel,
			 std::shared_ptr<BlockCache> _blockCache) 
	  : Node(_nodeId, _isAlive, _region, std::move(_blockchainManagementModel), _blockCache) {}