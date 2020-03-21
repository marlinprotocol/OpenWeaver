#ifndef EVENTMANAGER_H_
#define EVENTMANAGER_H_

#include <memory>
#include <queue> 
#include <unordered_map> 

#include "../EventQueue/EventQueue.h"
#include "../../Blockchain/Cache/BlockCache.h"
#include "../../Network/Network.h"
#include "../../../helpers/Logger/easylogging.h"

class EventManager {
private:
	static uint64_t currentTick;
    EventQueue eventQueue; 
    Network& network;
    std::shared_ptr<BlockCache> blockCache;

public:
	EventManager(Network& _network, std::shared_ptr<BlockCache> _blockCache);
	int addEvent(std::shared_ptr<Event> _event);
	// AsyncEvent getNextEvent() const;
	bool removeEvent(int _id);
	bool hasNextEvent();
	bool executeNextEvent();
	std::shared_ptr<BlockCache> getBlockCachePtr();
};

#endif /*EVENTMANAGER_H_*/ 