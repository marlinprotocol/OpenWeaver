#include "./RoutingTable.h"

RoutingTable::RoutingTable() {
	// numActiveConnections = 8;
}

bool RoutingTable::setNumActiveConnections(int _numActiveConnections) {
	numActiveConnections = _numActiveConnections;
	return true;
}

int RoutingTable::getNumActiveConnections() {
	return numActiveConnections;
}