#include "./Network.h"

bool Network::addNode(Node* node) {
	nodes.push_back(node);
	return true;
}

std::vector<Node*> Network::getNodes() const {
	return nodes;
}