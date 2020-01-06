#include "./EventManager.h"

uint64_t EventManager::currentTick = 0;

EventManager::EventManager(Network& _network, std::shared_ptr<BlockCache> _blockCache) : network(_network), blockCache(_blockCache) {}

int EventManager::addEvent(std::shared_ptr<Event> _event) {
	return eventQueue.addEvent(AsyncEvent(_event, currentTick + _event->getDurationInTicks()));
}

bool EventManager::removeEvent(int _id) {
	return eventQueue.removeEvent(AsyncEvent(_id));
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

	LOG(DEBUG) << "[Tickstamp: " << std::setw(10) << std::right << currentTick << "]";

	asyncEvent.execute(network, this, currentTick);

	return true;
}

std::shared_ptr<BlockCache>EventManager::getBlockCachePtr() {
	return blockCache;
}