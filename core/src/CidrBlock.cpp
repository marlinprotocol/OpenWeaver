#include "marlin/core/CidrBlock.hpp"

#include <cstring>
#include <sstream>
#include <arpa/inet.h>

namespace marlin {
namespace core {

CidrBlock::CidrBlock() : SocketAddress() {}

CidrBlock::CidrBlock(const CidrBlock &addr) : SocketAddress(addr) {}

CidrBlock CidrBlock::from_string(std::string blockString) {
	// TODO: Add support for other formats
	CidrBlock addr;
	inet_pton(AF_INET, blockString.substr(0,blockString.find("/")).c_str(), &reinterpret_cast<sockaddr_in *>(&addr)->sin_addr);
	reinterpret_cast<sockaddr_in *>(&addr)->sin_port = htons(std::stoi(blockString.substr(blockString.find("/")+1)));
	reinterpret_cast<sockaddr_in *>(&addr)->sin_family = AF_INET;
	return addr;
}

std::string CidrBlock::to_string() const {
	// TODO: Add support for other formats
	char buf[100];
	inet_ntop(AF_INET, &reinterpret_cast<const sockaddr_in *>(this)->sin_addr, buf, 100);

	std::stringstream blockString;
	blockString<<buf<<"/"<<ntohs(reinterpret_cast<const sockaddr_in *>(this)->sin_port);

	return blockString.str();
}

bool CidrBlock::does_contain_address(const SocketAddress &addr) const {
	char *_prefix = (char *)&(reinterpret_cast<const sockaddr_in *>(this)->sin_addr);
	uint16_t _prefix_length = ntohs(reinterpret_cast<const sockaddr_in *>(this)->sin_port);

	char *_addr = (char *)&(reinterpret_cast<const sockaddr_in *>(&addr)->sin_addr);

	for (int i=0; i<4 && _prefix_length>0; i++) {
		if (_prefix_length >= 8) {
			if (_addr[i] != _prefix[i]) {
				return false;
			}
		} else {
			if ((_addr[i]>>(8-_prefix_length)) != (_prefix[i]>>(8-_prefix_length))) {
				return false;
			}
		}
		_prefix_length -= 8;
	}

	return true;
}

} // namespace core
} // namespace marlin
