#include <queue> 
#include <unordered_map> 

#include "../Event/AsyncEvent.h"
#include "../../../helpers/Logger/easylogging.h"

using namespace std;

class EventQueue {
private:
    priority_queue<AsyncEvent> eventQueue; 

public:
	bool isEmpty();
	bool addEvent(AsyncEvent _event);
	AsyncEvent getNextEvent() const;
	bool removeNextEvent(); 
	// bool runNextEvent();
};
