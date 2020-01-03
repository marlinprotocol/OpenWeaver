#include "./NewBlockIdMessage.h"

NewBlockIdMessage::NewBlockIdMessage(int _blockId) : blockId(_blockId), Message(MessageType::NEW_BLOCK_ID) {}

int NewBlockIdMessage::getBlockId() {
	return blockId;
}

std::string NewBlockIdMessage::getType() {
	return "NewBlockIdMessage";
}