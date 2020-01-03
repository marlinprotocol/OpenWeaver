#include <memory>

#include "./Event.h"

class AsyncEvent {
private:
	std::shared_ptr<Event> event;
	long tickToExecOn;
	static int count;

public:
	AsyncEvent(std::shared_ptr<Event> _event, long _tickToExecOn);
	bool operator<(const AsyncEvent& e) const;
	long getTickToExecOn() const;
	bool execute();
};

