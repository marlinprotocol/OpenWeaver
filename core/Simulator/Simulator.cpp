#include <memory>

#include "./Simulator.h"
#include "../Blockchain/Block/Block.h"
#include "../Blockchain/Block/PoWBlock.h"
#include "../Blockchain/Block/PoSBlock.h"
#include "../EventManagement/Event/EventTypes/MessageToNodeEvent.h"
#include "../Network/Messages/Message.h"
#include "../Network/Messages/NewBlockIdMessage.h"
#include "../Network/Node/Miner.h"
#include "../../config/Config.h"
#include "../../helpers/InitializeNetwork.h"

// #include "../Networking/RoutingTable.h"


Simulator::Simulator() : eventManager(network) {
	blockCache = make_shared<BlockCache>();
}

int Simulator::createGenesisBlock() {
	int genesisBlockId;

	if(CONSENSUS_TYPE == "PoW") {
		shared_ptr<Block> genesisBlock = getGenesisPoWBlock();
		genesisBlockId = genesisBlock->getBlockId();
		blockCache->insert(genesisBlockId, genesisBlock);
	}

	LOG(INFO) << "[GenesisBlock Type " << CONSENSUS_TYPE << "created]";

	return genesisBlockId;
}

void Simulator::sendGenesisBlockToAllNodes(const Network& network, int genesisBlockId) {
	for(auto nodePtr: network.getNodes()) {
		int nodeId = nodePtr->getNodeId();
		eventManager.addEvent(shared_ptr<Event>(
								new MessageToNodeEvent( 
									std::shared_ptr<Message>(new NewBlockIdMessage(genesisBlockId)), nodeId, nodeId, 0
								)
							 ));
	}
	
	LOG(INFO) << "[GenesisBlockEvent added to all nodes]";
}

bool Simulator::setup() {
	network = getRandomNetwork(blockCache);

	int genesisBlockId = createGenesisBlock();
	sendGenesisBlockToAllNodes(network, genesisBlockId);

	return true;
}

void Simulator::start() {
	LOG(INFO) << "[Simulator started]"; 

	while(eventManager.hasNextEvent()) {
		eventManager.executeNextEvent();
	}

	LOG(INFO) << "[Simulator stopped]"; 
}