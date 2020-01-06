#include <string>

#include "./MessageToNodeEvent.h"
#include "../../../Network/Messages/NewBlockMinedMessage.h"

MessageToNodeEvent::MessageToNodeEvent(std::shared_ptr<Message> _message, int _forNodeId, int _fromNodeId, long long _durationInTicks)
				   : message(_message), forNodeId(_forNodeId), fromNodeId(_fromNodeId), Event(_durationInTicks) {}

bool MessageToNodeEvent::execute(Network& _network, EventManager* _eventManager, uint64_t _currentTick) {
	LOG(DEBUG) << "[" << std::setw(35) << std::left << "MessageToNodeEvent::execute]"
			   << "[From:" << std::setw(8) << std::right << std::to_string(fromNodeId) << "]"
			   << "[To:" << std::setw(8) << std::right << std::to_string(forNodeId) << "]"
			   << "[Type:" << std::setw(15) << std::right << message->getType() << "]";

	std::shared_ptr<Node> node = _network.getNodes()[forNodeId];

	switch(message->getMessageType()) {
		case MessageType::NEW_BLOCK_ID:
			node->onNewBlockIdMessage(std::dynamic_pointer_cast<NewBlockIdMessage>(message), _eventManager);
			break;
		case MessageType::NEW_BLOCK_MINED:
			node->onNewBlockMinedMessage(std::dynamic_pointer_cast<NewBlockMinedMessage>(message), _eventManager, _currentTick);
			break;	
		case MessageType::NEW_BLOCK_BODY:
			break;
	}	

	return true;
}