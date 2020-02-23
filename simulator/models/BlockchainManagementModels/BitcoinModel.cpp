#include "./BitcoinModel.h"
#include "../../core/Blockchain/Block/PoWBlock.h"
#include "../../core/Network/Node/Miner.h"
#include "../../core/Network/Node/Node.h"

BitcoinModel::BitcoinModel(Network &_network): exp(1.0/600), unif(0, 1), network(_network) {
	// initiaze random number generator, seed fixed to 22 to make it deterministic across runs
	// else make it time-dependent for randomness	
	uint64_t seed = 22;
	std::seed_seq ss{uint32_t(seed & 0xffffffff), uint32_t(seed>>32)};
    rng.seed(ss);
}

void BitcoinModel::OnOutOfRangeNewBlockArrival() {

}

uint64_t BitcoinModel::getRandomNumFromExponentialDistribution() {
    return (uint64_t) exp(rng);
}

uint64_t BitcoinModel::getNextBlockTime() {
	return getRandomNumFromExponentialDistribution();
}

void BitcoinModel::getNextBlockProducer() {

	long long totalHashPower = 0l;

	for(std::shared_ptr<Node> node: network.getNodes()) {
		std::shared_ptr<Miner> miner = std::static_pointer_cast<Miner>(node);
		totalHashPower += miner->getHashPower();
	}

	std::vector<double> cumulativeProbabilities;

	// compute cumulative probability vector of node distribution in regions to randomly assign a node's region
	// for(int i=0; i<NUM_REGIONS; i++) {
	// 	if(i==0)
	// 		cumulativeProbabilities.push_back(REGION_DISTRIBUTION_OF_NODES[0]);
	// 	else
	// 		cumulativeProbabilities.push_back(
	// 			cumulativeProbabilities[i-1] + REGION_DISTRIBUTION_OF_NODES[i]);
	// }
}

std::shared_ptr<Block> BitcoinModel::createGenesisBlock() {
	int genesisBlockId;
	
	std::shared_ptr<Block> genesisBlock = getGenesisPoWBlock();

	LOG(INFO) << "[GenesisBlock Bitcoin with id " << genesisBlock->getBlockId() << " created]";

	return genesisBlock;
}