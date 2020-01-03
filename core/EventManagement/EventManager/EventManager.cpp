#include "./EventManager.h"

int EventManager::currentTick = 0;

EventManager::EventManager(Network& _network) : network(_network) {}

bool EventManager::addEvent(std::shared_ptr<Event> _event) {
	eventQueue.addEvent(AsyncEvent(_event, currentTick + _event->getDurationInTicks()));
	return true;
}

bool EventManager::hasNextEvent() {
	return !eventQueue.isEmpty();
}

bool EventManager::executeNextEvent() {
	LOG(DEBUG) << "[" << std::setw(35) << std::left << "EventManager::executeNextEvent]";

	if(eventQueue.isEmpty()) {
		LOG(DEBUG) << "[EventQueue empty]";
		return false;
	}
	
	AsyncEvent asyncEvent = eventQueue.getNextEvent();
	eventQueue.removeNextEvent();

	currentTick = asyncEvent.getTickToExecOn();

	asyncEvent.execute(network);

	return true;
}