#include "./Network.h"

bool Network::addNode(std::shared_ptr<Node> node) {
	nodes.push_back(node);
	return true;
}

std::vector<std::shared_ptr<Node>> Network::getNodes() const {
	return nodes;
}