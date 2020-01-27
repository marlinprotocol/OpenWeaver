#ifndef NEWBLOCKMINEDMESSAGE_H_
#define NEWBLOCKMINEDMESSAGE_H_

#include <memory>

#include "./Message.h"
#include "../../Blockchain/Cache/BlockCache.h"

class NewBlockMinedMessage : public Message {
private:
	std::shared_ptr<Block> newBlock;
	int parentBlockId;

public:
	NewBlockMinedMessage(int _parentBlockId);
	int createNewBlockObject(std::shared_ptr<BlockCache> _blockCache, int _blockProducerId, uint64_t _tickStamp);
	int getBlockId();
	std::string getType();
};

#endif /*NEWBLOCKMINEDMESSAGE_H_*/