#ifndef MESSAGETONODEEVENT_H_
#define MESSAGETONODEEVENT_H_

#include <memory>

#include "../Event.h"
#include "../../../Network/Messages/Message.h"

class MessageToNodeEvent : public Event {
private:
	std::shared_ptr<Message> message;
	int forNodeId;
	int fromNodeId;

public: 
	MessageToNodeEvent(std::shared_ptr<Message> _message, int _forNodeId, int _fromNodeId);
	bool execute();
};

#endif /*MESSAGETONODEEVENT_H_*/