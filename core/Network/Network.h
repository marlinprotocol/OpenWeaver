#ifndef NETWORK_H_
#define NETWORK_H_

#include <memory>
#include <vector>

#include "./Node/Node.h"
#include "../../helpers/Logger/easylogging.h"

class Network {
private:
	std::vector<std::shared_ptr<Node>> nodes;

public:
	bool addNode(std::shared_ptr<Node> node);
	std::vector<std::shared_ptr<Node>> getNodes() const;
};

#endif /*NETWORK_H_*/
