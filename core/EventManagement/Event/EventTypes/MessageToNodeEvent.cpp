#include "./MessageToNodeEvent.h"

MessageToNodeEvent::MessageToNodeEvent(std::shared_ptr<Message> _message, int _forNodeId, int _fromNodeId)
				   : message(_message), forNodeId(_forNodeId), fromNodeId(_fromNodeId) {}

bool MessageToNodeEvent::execute() {
	return true;
}