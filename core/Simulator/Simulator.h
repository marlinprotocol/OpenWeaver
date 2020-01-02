#ifndef SIMULATOR_H_
#define SIMULATOR_H_

#include "../Network/Network.h"

class Simulator {
private:
	Network network;

public:
	bool setup();
	void start();
};

#endif /*SIMULATOR_H_*/
