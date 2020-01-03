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
	long tickToExecOn;
	static int count;
	int id;

public:
	AsyncEvent(int _id); // only used to generate a fake async event reqd when deleting an event from priority queue by id
	AsyncEvent(std::shared_ptr<Event> _event, long _tickToExecOn);
	bool operator<(const AsyncEvent& e) const;
	bool operator==(const AsyncEvent& e) const;
	long getTickToExecOn() const;
	bool execute(Network& _network, EventManager* _eventManager);	
};

#endif /*ASYNCEVENT_H_*/