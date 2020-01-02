#ifndef INITIALIZENETWORK_H_
#define INITIALIZENETWORK_H_

#include <iomanip>
#include <random>
#include <vector>

#include "../config/Config.h"
#include "../core/Network/Network.h"
#include "../core/Network/Node/Node.h"
#include "./Center.h"
#include "./Logger/easylogging.h"

bool generateNodes(Network& network) {
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
				network.addNode(new Node(i, true, j));
				break;
			}
		}
	}

	return true;
}

bool testSanity(Network& network) {
	LOG(INFO) << "Testing sanity of generated network topology"; 

	int numNodesPerRegion[NUM_REGIONS] = {0};

	const std::vector<Node*> nodes = network.getNodes();

	for(auto node: nodes) {
		numNodesPerRegion[node->getRegion()]++;
	}

	el::Logger* networkTopologyLogger = el::Loggers::getLogger("networkTopology");

	CLOG(INFO, "networkTopology") << std::setw(20) << centered("REGION")
								  << std::setw(20) << centered("EXPECTED")
								  << std::setw(20) << centered("GENERATED");	

	for(int i=0; i<NUM_REGIONS; i++) {
		CLOG(INFO, "networkTopology") << std::setw(20) << std::left << REGIONS[i]
								  	  << std::setw(20) << std::right << REGION_DISTRIBUTION_OF_NODES[i] * NUM_NODES
								 	  << std::setw(20) << std::right << numNodesPerRegion[i];
	}

	LOG(INFO) << "Sanity check of generated network topology successful"; 

	return true;
}

Network getRandomNetwork() {
	Network network;

	generateNodes(network);
	testSanity(network);
	// generateLinks();

	return network;
}

#endif /*INITIALIZENETWORK_H_*/
