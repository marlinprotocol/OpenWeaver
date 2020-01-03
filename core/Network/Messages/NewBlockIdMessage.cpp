#include "./NewBlockIdMessage.h"

NewBlockIdMessage::NewBlockIdMessage(int _blockId) : blockId(_blockId) {}

int NewBlockIdMessage::getBlockId() {
	return blockId;
}