#ifndef NEWBLOCKMINEDMESSAGE_H_
#define NEWBLOCKMINEDMESSAGE_H_

#include <memory>

#include "./Message.h"
#include "../../Blockchain/Cache/BlockCache.h"

class NewBlockMinedMessage : public Message {
public:
	NewBlockMinedMessage();
	std::string getType();
};

#endif /*NEWBLOCKMINEDMESSAGE_H_*/