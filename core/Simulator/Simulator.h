#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include <memory>

#include "../Blockchain/Cache/BlockCache.h"
#include "../Network/Network.h"

class Simulator {
private:
	std::shared_ptr<BlockCache> blockCache;
	Network network;

public:
	Simulator();
	bool setup();
	void start();
};

#endif /*SIMULATOR_H_*/
