#include "./Miner.h"

Miner::Miner(int _nodeId, bool _isAlive, int _region, 
			 std::unique_ptr<BlockchainManagementModel> _blockchainManagementModel,
			 std::shared_ptr<BlockCache> _blockCache) 
	  : Node(_nodeId, _isAlive, _region, std::move(_blockchainManagementModel), _blockCache) {}

void Miner::onNewBlockIdMessage(std::shared_ptr<NewBlockIdMessage> _message) {
	LOG(DEBUG) << "[" << std::setw(35) << std::left << "Miner::onNewBlockIdMessage]" 
			   << "[NodeId:" << std::setw(6) << std::right << std::to_string(getNodeId()) << "]";

	blockchain.addBlock(_message->getBlockId(), _message->getBlockId());
}