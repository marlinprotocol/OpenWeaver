#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "./MessageType.h"
#include "../../../helpers/Logger/easylogging.h"

class Message {
private:
	MessageType messageType;

public:
	Message(MessageType _messageType);
	virtual std::string getType() = 0;
	virtual ~Message() {};
	MessageType getMessageType();
};

#endif /*MESSAGE_H_*/