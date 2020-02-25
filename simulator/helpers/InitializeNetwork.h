#ifndef INITIALIZENETWORK_H_
#define INITIALIZENETWORK_H_

#include <memory>
#include <random>
#include <string>
#include <vector>

#include "./Center.h"
#include "./Logger/easylogging.h"
#include "../config/Config.h"
#include "../core/Blockchain/Cache/BlockCache.h"
#include "../core/Network/Network.h"
#include "../core/Network/Node/Miner.h"
#include "../core/Network/Node/Node.h"
#include "../core/Network/Node/NodeType.h"
#include "../core/Networking/RoutingTable.h"
#include "../models/BlockchainManagementModels/BitcoinModel.h"

bool generateNodes(Network& network, std::shared_ptr<BlockCache> blockCache, NodeType nodeType,
				   std::shared_ptr<BlockchainManagementModel> _blockchainManagementModel) {
	std::vector<double> cumulativeProbabilities;

	// compute cumulative probability vector of node distribution in regions to randomly assign a node's region
	for(int i=0; i<NUM_REGIONS; i++) {
		if(i==0)
			cumulativeProbabilities.push_back(REGION_DISTRIBUTION_OF_NODES[0]);
		else
			cumulativeProbabilities.push_back(
				cumulativeProbabilities[i-1] + REGION_DISTRIBUTION_OF_NODES[i]);
	}

	if(cumulativeProbabilities[NUM_REGIONS-1] != 1) {
		LOG(FATAL) << "Probabilities in REGION_DISTRIBUTION_OF_NODES do not add up to 1. Sum obtained: " 
				   << cumulativeProbabilities[NUM_REGIONS-1];
		return false;
	}

	// initiaze random number generator, seed fixed to 2121 to make it deterministic across runs
	// else make it time-dependent for randomness
	std::mt19937_64 rng;
	uint64_t seed = 2121;
	std::seed_seq ss{uint32_t(seed & 0xffffffff), uint32_t(seed>>32)};
    rng.seed(ss);
    std::uniform_real_distribution<double> unif(0, 1);

    // create Nodes and assign their regions based on the index of generated random number in the cumulative probability vector
	for(int i=0; i<NUM_NODES; i++) {
		double randomNumber = unif(rng);
		for(int j=0; j<NUM_REGIONS; j++) {
			if(randomNumber < cumulativeProbabilities[j]) {
				if(nodeType == NodeType::Miner) 
					network.addNode( std::shared_ptr<Node>( 
										new Miner(i, true, j, 
											      _blockchainManagementModel,													
												  blockCache, 1) 
												          ) );
				break;
			}
		}
	}

	return true;
}

bool printGeneratedNetwork(const Network& network) {
	LOG(INFO) << "[PRINT_GENERATED_NETWORK_START]"; 

	int numNodesPerRegion[NUM_REGIONS] = {0};

	const std::vector<std::shared_ptr<Node>> nodes = network.getNodes();

	for(auto node: nodes) {
		numNodesPerRegion[node->getRegion()]++;
	}

	el::Logger* networkTopologyLogger = el::Loggers::getLogger("networkTopology");

	CLOG(INFO, "networkTopology") << std::setw(13) << centered("REGION_ID")
								  << std::setw(20) << centered("REGION")
								  << std::setw(20) << centered("EXPECTED")
								  << std::setw(20) << centered("GENERATED");	

	for(int i=0; i<NUM_REGIONS; i++) {
		CLOG(INFO, "networkTopology") << std::setw(13) << centered(std::to_string(i))
									  << std::setw(20) << centered(REGIONS[i])
								  	  << std::setw(20) << centered(std::to_string(REGION_DISTRIBUTION_OF_NODES[i] * NUM_NODES))
								 	  << std::setw(20) << centered(std::to_string(numNodesPerRegion[i]));
	}

	LOG(INFO) << "[PRINT_GENERATED_NETWORK_END]"; 

	return true;
}

bool initializeRoutingTable(const Network& network) {
	LOG(INFO) << "[INITIALIZE_ROUTING_TABLE_START]"; 

	const std::vector<std::shared_ptr<Node>> nodes = network.getNodes();

	for(auto node: nodes) {
		
	}	

	LOG(INFO) << "[INITIALIZE_ROUTING_TABLE_END]"; 

	return true;
}

Network getRandomNetwork(std::shared_ptr<BlockCache> _blockCache, 
						 std::shared_ptr<BlockchainManagementModel> _blockchainManagementModel) {
	Network network;

	generateNodes(network, _blockCache, NodeType::Miner, _blockchainManagementModel);
	printGeneratedNetwork(network);

	initializeRoutingTable(network);

	return network;
}

void sendGenesisBlockToAllNodes(const Network& network, int _genesisBlockId, EventManager& eventManager) {
	for(auto nodePtr: network.getNodes()) {
		int nodeId = nodePtr->getNodeId();
		eventManager.addEvent(std::shared_ptr<Event>(
								new MessageToNodeEvent( 
									std::shared_ptr<Message>(new NewBlockIdMessage(_genesisBlockId)), 
									nodeId, -1, 0
								)
							 ));
	}
	
	LOG(INFO) << "[GenesisBlockEvent added to all nodes]";
}

void scheduleNextBlock(EventManager& eventManager, uint64_t _firstBlockInterval, 
					   std::shared_ptr<Node> _firstBlockProducer, int _genesisBlockId) {
	int nodeId = _firstBlockProducer->getNodeId();

	eventManager.addEvent(std::shared_ptr<Event>(
								new MessageToNodeEvent( 
									std::shared_ptr<Message>(new NewBlockMinedMessage()), 
									nodeId, nodeId, _firstBlockInterval
								)
						  ));

	LOG(INFO) << "[Block 1 added to Node: " << nodeId << " at tickstamp: " << _firstBlockInterval << "]";
}

void scheduleBlockProduction(std::shared_ptr<BlockchainManagementModel> _blockchainManagementModel,
							 const Network& _network, std::shared_ptr<BlockCache> _blockCache,
							 EventManager& eventManager) {
	std::shared_ptr<Block> genesisBlock = _blockchainManagementModel->createGenesisBlock();
	_blockCache->insert(genesisBlock);

	sendGenesisBlockToAllNodes(_network, genesisBlock->getBlockId(), eventManager);

	uint64_t firstBlockInterval = _blockchainManagementModel->getNextBlockTime();
	std::shared_ptr<Node> firstBlockProducer = _blockchainManagementModel->getNextBlockProducer();

	scheduleNextBlock(eventManager, firstBlockInterval, firstBlockProducer, genesisBlock->getBlockId());
}

#endif /*INITIALIZENETWORK_H_*/