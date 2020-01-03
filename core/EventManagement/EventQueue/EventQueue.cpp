#include "./EventQueue.h"

bool EventQueue::isEmpty() {
	return eventQueue.empty();
}

bool EventQueue::addEvent(AsyncEvent _event) {
	eventQueue.push(_event);
	return true;
}

// bool EventQueue::runNextEvent() {
// 	return true;
// }

AsyncEvent EventQueue::getNextEvent() const {
	return eventQueue.top();
}

bool EventQueue::removeNextEvent() {
	// if(eventQueue.empty()) return false;
	eventQueue.pop();
	return true;
}	

bool EventQueue::removeEvent(AsyncEvent _event) {
	return eventQueue.remove(_event);
}

bool EventQueue::removeEvent(int _id) {
	return removeEvent(AsyncEvent(_id));
}