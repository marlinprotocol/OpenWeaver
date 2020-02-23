#include "./BitcoinModel.h"
#include "../../core/Blockchain/Block/PoWBlock.h"
#include "../../core/Network/Node/Miner.h"

BitcoinModel::BitcoinModel(Network &_network): exp(1.0/600), unif(0, 1), network(_network) {
	// initiaze random number generator, seed fixed to 22 to make it deterministic across runs
	// else make it time-dependent for randomness	
	uint64_t seed = 22;
	std::seed_seq ss{uint32_t(seed & 0xffffffff), uint32_t(seed>>32)};
    rng.seed(ss);
}

void BitcoinModel::OnOutOfRangeNewBlockArrival() {

}

uint64_t BitcoinModel::getNextBlockTime() {
	return (uint64_t) exp(rng);
}

std::shared_ptr<Node> BitcoinModel::getNextBlockProducer() {
	LOG(INFO) << "[BitcoinModel::getNextBlockProducer start]";

	std::vector<double> hashpowerFraction;
	long long totalHashPower = 0l;
	std::shared_ptr<Node> blockProducer;

	for(int i=0; i<network.getNodes().size(); i++) {
		std::shared_ptr<Miner> miner = std::static_pointer_cast<Miner>(network.getNodes()[i]);
		
		if(i==0)
			hashpowerFraction.push_back(miner->getHashPower());
		else
			hashpowerFraction.push_back(
				hashpowerFraction[i-1] + miner->getHashPower());

		totalHashPower += miner->getHashPower();
	}

	for(int i=0; i<network.getNodes().size(); i++) {
		hashpowerFraction[i] /= totalHashPower;
		LOG(DEBUG) << "[" << std::setw(35) << std::left << "BitcoinModel::getNextBlockProducer]" 
			       << "[NodeId: " << i << ", CumulativeHash: "  << hashpowerFraction[i] << "]";
	}

	double randomNumber = unif(rng);
	for(int i=0; i<network.getNodes().size(); i++) {
		if(randomNumber < hashpowerFraction[i]) {
			blockProducer = network.getNodes()[i];
			break;
		}
	}

	LOG(INFO) << "[BitcoinModel::getNextBlockProducer end]";

	return blockProducer;
}

std::shared_ptr<Block> BitcoinModel::createGenesisBlock() {
	int genesisBlockId;
	
	std::shared_ptr<Block> genesisBlock = getGenesisPoWBlock();

	LOG(INFO) << "[GenesisBlock Bitcoin with id " << genesisBlock->getBlockId() << " created]";

	return genesisBlock;
}