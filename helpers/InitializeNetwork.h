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
#include "../models/BlockchainManagementModels/BlockchainManagementBitcoinModel.h"

bool generateNodes(Network& network, std::shared_ptr<BlockCache> blockCache, NodeType nodeType) {
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
											      std::unique_ptr<BlockchainManagementBitcoinModel>(new BlockchainManagementBitcoinModel()),													
												  blockCache) 
												          ) );
				break;
			}
		}
	}

	return true;
}

bool printGeneratedNetwork(Network& network) {
	LOG(INFO) << "[PRINTING_GENERATED_NETWORK_START]"; 

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

	LOG(INFO) << "[PRINTING_GENERATED_NETWORK_END]"; 

	return true;
}

Network getRandomNetwork(std::shared_ptr<BlockCache> blockCache) {
	Network network;

	generateNodes(network, blockCache, NodeType::Miner);
	printGeneratedNetwork(network);

	return network;
}

#endif /*INITIALIZENETWORK_H_*/
