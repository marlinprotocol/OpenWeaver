#include "./Message.h"

Message::Message(MessageType _messageType) : messageType(_messageType) {}

MessageType Message::getMessageType() {
	return messageType;
}