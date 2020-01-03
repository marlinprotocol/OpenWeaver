#include "./Miner.h"

Miner::Miner(int _nodeId, bool _isAlive, int _region, 
			 std::unique_ptr<BlockchainManagementModel> _blockchainManagementModel,
			 std::shared_ptr<BlockCache> _blockCache) 
	  : Node(_nodeId, _isAlive, _region, std::move(_blockchainManagementModel), _blockCache), miningEventId(-1) {}

void Miner::onNewBlockIdMessage(std::shared_ptr<NewBlockIdMessage> _message, EventManager* _eventManager) {
	LOG(DEBUG) << "[" << std::setw(35) << std::left << "Miner::onNewBlockIdMessage]" 
			   << "[NodeId:" << std::setw(6) << std::right << std::to_string(getNodeId()) << "]";

	int prevBlockchainHeight = blockchain.getBlockchainHeight();

	blockchain.addBlock(_message->getBlockId(), _message->getBlockId());

	int newBlockchainHeight = blockchain.getBlockchainHeight();

	if(newBlockchainHeight == prevBlockchainHeight + 1) {
		if(miningEventId != -1) {
			_eventManager->removeEvent(miningEventId);
		}
	}

}