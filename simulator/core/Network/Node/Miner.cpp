#include <memory>

#include "./Miner.h"
#include "../Messages/NewBlockMinedMessage.h"
#include "../../EventManagement/Event/EventTypes/MessageToNodeEvent.h"

Miner::Miner(int _nodeId, bool _isAlive, int _region, 
			 std::shared_ptr<BlockchainManagementModel> _blockchainManagementModel,
			 std::shared_ptr<BlockCache> _blockCache) 
	  : Node(_nodeId, _isAlive, _region, _blockchainManagementModel, _blockCache), miningEventId(-1) {
}

long long Miner::getHashPower() {
	return hashpower;
}

void Miner::onNewBlockIdMessage(std::shared_ptr<NewBlockIdMessage> _message, EventManager* _eventManager) {
	LOG(DEBUG) << "[" << std::setw(35) << std::left << "Miner::onNewBlockIdMessage]" 
			   << "[NodeId:" << std::setw(6) << std::right << std::to_string(getNodeId()) << "]";

	int prevBlockchainHeight = blockchain.getBlockchainHeight();

	int newBlockHeight = _eventManager->getBlockCachePtr()->getBlockById(_message->getBlockId())->getBlockHeight();

	blockchain.addBlock(newBlockHeight, _message->getBlockId());

	// int newBlockchainHeight = blockchain.getBlockchainHeight();

	// if(newBlockchainHeight == prevBlockchainHeight + 1) {
	// 	if(miningEventId != -1) {
	// 		_eventManager->removeEvent(miningEventId);
	// 	}

	// 	miningEventId = _eventManager->addEvent(std::shared_ptr<Event>(new MessageToNodeEvent( 
	// 															 	 std::shared_ptr<Message>(new NewBlockMinedMessage(_message->getBlockId())), 
	// 																 getNodeId(), getNodeId(), getTicksUntilNextBlock()
	// 															  )));
	// }
}

void Miner::onNewBlockMinedMessage(std::shared_ptr<NewBlockMinedMessage> _message, EventManager* _eventManager, uint64_t _currentTick) {
	LOG(DEBUG) << "[" << std::setw(35) << std::left << "Miner::onNewBlockMinedMessage]" 
			   << "[NodeId:" << std::setw(6) << std::right << std::to_string(getNodeId()) << "]";

	int blockId = _message->createNewBlockObject(_eventManager->getBlockCachePtr(), getNodeId(), _currentTick);

	int newBlockHeight = _eventManager->getBlockCachePtr()->getBlockById(_message->getBlockId())->getBlockHeight();

	blockchain.addBlock(newBlockHeight, _message->getBlockId());

	// miningEventId = _eventManager->addEvent(std::shared_ptr<Event>(new MessageToNodeEvent( 
	// 														 	 std::shared_ptr<Message>(new NewBlockMinedMessage(_message->getBlockId())), 
	// 															 getNodeId(), getNodeId(), getTicksUntilNextBlock()
	// 														  )));
}