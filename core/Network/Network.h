#ifndef NETWORK_H_
#define NETWORK_H_

#include <vector>

#include "./Node/Node.h"

class Network {
private:
	std::vector<Node*> nodes;

public:
	bool addNode(Node* node);
	std::vector<Node*> getNodes() const;
};

#endif /*NETWORK_H_*/
