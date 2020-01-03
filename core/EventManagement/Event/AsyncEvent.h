#include "./Event.h"

class AsyncEvent {
private:
	Event event;
	long tickToExecOn;
	static int count;

public:
	AsyncEvent(Event _event, long _tickToExecOn);
	bool operator<(const AsyncEvent& e) const;
	long getTickToExecOn() const;
	bool execute();
};

