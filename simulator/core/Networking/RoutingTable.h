#include <set>
#include <unordered_map>

#include "../../helpers/Logger/easylogging.h"

class RoutingTable {
private:
	int nodeOwnerId;
	int numActiveConnections;
	std::unordered_map<int, int> nodeIdLatencyMap;
	std::set<int> activePeers;

public:
	RoutingTable();
	bool setNumActiveConnections(int _numActiveConnections);
	int getNumActiveConnections();
	bool addNeighbour(int nodeId);
	bool removeNeighbour(int nodeId);
};