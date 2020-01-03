#ifndef EVENTMANAGER_H_
#define EVENTMANAGER_H_

#include <queue> 
#include <unordered_map> 

#include "../EventQueue/EventQueue.h"
#include "../../Network/Network.h"
#include "../../../helpers/Logger/easylogging.h"

using namespace std;

class EventManager {
private:
	static int currentTick;
    EventQueue eventQueue; 
    Network& network;

public:
	EventManager(Network& _network);
	bool addEvent(shared_ptr<Event> _event);
	// AsyncEvent getNextEvent() const;
	bool removeEvent(int _id);
	bool hasNextEvent();
	bool executeNextEvent();
};

#endif /*EVENTMANAGER_H_*/
