#include "./Simulator.h"
#include "../../helpers/InitializeNetwork.h"

// #include "../EventManagement/EventManager/EventManager.h"
// #include "../Networking/RoutingTable.h"
// #include "../Blockchain/Block/Block.h"
// #include "../Blockchain/Block/PoWBlock.h"
// #include "../Blockchain/Block/PoSBlock.h"

bool Simulator::setup() {
	network = getRandomNetwork();
	return true;
}

void Simulator::start() {
	
}