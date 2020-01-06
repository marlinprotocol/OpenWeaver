#ifndef ASYNCEVENT_H_
#define ASYNCEVENT_H_

#include <memory>

#include "./Event.h"
#include "../../Network/Network.h"
#include "../../../helpers/Logger/easylogging.h"

class EventManager;

class AsyncEvent {
private:
	std::shared_ptr<Event> event;
	uint64_t tickToExecOn;
	static int count;
	int id;

public:
	AsyncEvent(int _id); // only used to generate a fake async event reqd when deleting an event from priority queue by id
	AsyncEvent(std::shared_ptr<Event> _event, uint64_t _tickToExecOn);
	bool operator<(const AsyncEvent& e) const;
	bool operator==(const AsyncEvent& e) const;
	long getTickToExecOn() const;
	bool execute(Network& _network, EventManager* _eventManager, uint64_t _currentTick);	
	int getEventId() const;
};

#endif /*ASYNCEVENT_H_*/