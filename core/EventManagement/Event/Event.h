#ifndef EVENT_H_
#define EVENT_H_

#include "./EventTypes/EventType.h"
#include "../../Network/Network.h"
#include "../../../helpers/Logger/easylogging.h"

class Event {
private:
	long long durationInTicks;

public:
	Event(long long _durationInTicks);
	long getDurationInTicks();
	virtual bool execute(Network& _network) = 0;
	virtual ~Event() {}
};

#endif /*EVENT_H_*/