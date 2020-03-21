#ifndef ROUTINGTABLE_H_
#define ROUTINGTABLE_H_

#include <set>

#include "../../helpers/Logger/easylogging.h"

class RoutingTable {
private:
	int nodeOwnerId;
	int maxOutConnections;
	int maxInConnections;
	std::set<int> outConnections;
	std::set<int> inConnections;

public:
	RoutingTable();
	bool setup();
	int getOutConnections();
	int getInConnections();
	bool setMaxOutConnections(int _maxOutConnections);
	int getMaxOutConnections();
	bool setMaxInConnections(int _maxInConnections);
	int getMaxInConnections();
	bool addOutConnection(int nodeId);
	bool removeOutConnection(int nodeId);
	bool addInConnection(int nodeId);
	bool removeInConnection(int nodeId);
};

#endif /*ROUTINGTABLE_H_*/