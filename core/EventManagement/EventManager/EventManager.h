#include <queue> 
#include <unordered_map> 

#include "../EventQueue/EventQueue.h"

using namespace std;

class EventManager {
private:
	static int currentTick;
    EventQueue eventQueue; 

public:
	bool addEvent(Event _event);
	// AsyncEvent getNextEvent() const;
	// bool removeEvent(); // when to use: when link fails and message wont reach other end?
	bool hasNextEvent();
	bool executeNextEvent();
};
