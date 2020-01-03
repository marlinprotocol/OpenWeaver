#ifndef EVENTQUEUE_H_
#define EVENTQUEUE_H_

#include <queue> 
#include <unordered_map> 

#include "../Event/AsyncEvent.h"
#include "../../../helpers/Logger/easylogging.h"

template<typename T>
class PriorityQueue : public std::priority_queue<T, std::vector<T>> {
public:
    bool remove(T key) {
        auto it = std::find(this->c.begin(), this->c.end(), key);
        
        if (it == this->c.end()) 
        	return false;
        
        this->c.erase(it);
        std::make_heap(this->c.begin(), this->c.end(), this->comp);
        return true;
 	}
};

class EventQueue {
private:
    PriorityQueue<AsyncEvent> eventQueue; 

public:
	bool isEmpty();
	bool addEvent(AsyncEvent _event);
	AsyncEvent getNextEvent() const;
	bool removeNextEvent(); 
	bool removeEvent(AsyncEvent _event);
	bool removeEvent(int _id);
};

#endif /*EVENTQUEUE_H_*/