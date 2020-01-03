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
	// bool removeEvent(); // when to use: when link fails and message wont reach other end?
	bool hasNextEvent();
	bool executeNextEvent();
};
