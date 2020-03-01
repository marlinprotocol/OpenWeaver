#include "./NewBlockMinedMessage.h"
#include "../../Blockchain/Block/Block.h"
#include "../../Blockchain/Block/PoWBlock.h"

NewBlockMinedMessage::NewBlockMinedMessage() : Message(MessageType::NEW_BLOCK_MINED) {
}

std::string NewBlockMinedMessage::getType() {
	return "NewBlockMinedMessage";
}