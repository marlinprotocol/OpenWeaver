#include "marlin/core/SocketAddress.hpp"

#include <cstring>
#include <sstream>
#include <arpa/inet.h>

namespace marlin {
namespace core {

SocketAddress::SocketAddress() {
	memset(this, 0, sizeof(SocketAddress));
	reinterpret_cast<sockaddr_in *>(this)->sin_family = AF_INET;
}

SocketAddress::SocketAddress(const SocketAddress &addr) {
	memcpy(this, &addr, sizeof(SocketAddress));
}

SocketAddress &SocketAddress::operator=(const SocketAddress &addr) {
	memcpy(this, &addr, sizeof(SocketAddress));
	return *this;
}

SocketAddress::SocketAddress(const sockaddr_storage &addr) {
	memcpy(this, &addr, sizeof(sockaddr_storage));
}

SocketAddress &SocketAddress::operator=(const sockaddr_storage &addr) {
	memcpy(this, &addr, sizeof(sockaddr_storage));
	return *this;
}

SocketAddress::SocketAddress(const sockaddr &addr) {
	memcpy(this, &addr, sizeof(sockaddr));
}

SocketAddress &SocketAddress::operator=(const sockaddr &addr) {
	memcpy(this, &addr, sizeof(sockaddr));
	return *this;
}

SocketAddress::SocketAddress(const sockaddr_in &addr) {
	memcpy(this, &addr, sizeof(sockaddr_in));
}

SocketAddress &SocketAddress::operator=(const sockaddr_in &addr) {
	memcpy(this, &addr, sizeof(sockaddr_in));
	return *this;
}

SocketAddress::SocketAddress(const sockaddr_in6 &addr) {
	memcpy(this, &addr, sizeof(sockaddr_in6));
}

SocketAddress &SocketAddress::operator=(const sockaddr_in6 &addr) {
	memcpy(this, &addr, sizeof(sockaddr_in6));
	return *this;
}

SocketAddress SocketAddress::from_string(std::string addrString) {
	// TODO: Add support for other formats
	SocketAddress addr;
	inet_pton(AF_INET, addrString.substr(0,addrString.find(":")).c_str(), &reinterpret_cast<sockaddr_in *>(&addr)->sin_addr);
	reinterpret_cast<sockaddr_in *>(&addr)->sin_port = htons(std::stoi(addrString.substr(addrString.find(":")+1)));
	reinterpret_cast<sockaddr_in *>(&addr)->sin_family = AF_INET;
	return addr;
}

std::string SocketAddress::to_string() const {
	// TODO: Add support for other formats
	char buf[100];
	inet_ntop(AF_INET, &reinterpret_cast<const sockaddr_in *>(this)->sin_addr, buf, 100);

	std::stringstream addrString;
	addrString<<buf<<":"<<ntohs(reinterpret_cast<const sockaddr_in *>(this)->sin_port);

	return addrString.str();
}

std::string SocketAddress::ip_string() const {
	// TODO: Add support for other formats
	char buf[100];
	inet_ntop(AF_INET, &reinterpret_cast<const sockaddr_in *>(this)->sin_addr, buf, 100);

	return std::string(buf);
}

uint16_t SocketAddress::get_port() const {
	return ntohs(reinterpret_cast<const sockaddr_in *>(this)->sin_port);
}

void SocketAddress::set_port(uint16_t port) {
	reinterpret_cast<sockaddr_in *>(this)->sin_port = ntohs(port);
}

SocketAddress SocketAddress::loopback_ipv4(uint16_t port) {
	return from_string(std::string("127.0.0.1:").append(std::to_string(port)));
}

// TODO - Temporary hack - previously used to memcmp bytes directly which wasn't working
// Possibly because struct isn't zeroed out entirely so "unused" bytes have random data
bool SocketAddress::operator==(const SocketAddress &other) const {
	return this->to_string() == other.to_string();
}

// TODO - Temporary hack - previously used to memcmp bytes directly which wasn't working
// Possibly because struct isn't zeroed out entirely so "unused" bytes have random data
bool SocketAddress::operator<(const SocketAddress &other) const {
	return this->to_string() < other.to_string();
}

size_t SocketAddress::serialize(uint8_t* bytes, size_t size) const {
	if(size < 8) {
		return 0;
	}

	uint16_t family = reinterpret_cast<const sockaddr_in *>(this)->sin_family;
	uint8_t *start = (uint8_t *)&(reinterpret_cast<const sockaddr_in *>(this)->sin_addr);
	uint16_t port = reinterpret_cast<const sockaddr_in *>(this)->sin_port;

	bytes[0] = static_cast<uint8_t>(family >> 8);
	bytes[1] = static_cast<uint8_t>(family & 0xff);
	std::memcpy(bytes+2, start, 4);
	bytes[6] = port >> 8;
	bytes[7] = port & 0xff;

	return 8;
}

SocketAddress SocketAddress::deserialize(uint8_t const* bytes, size_t size) {
	SocketAddress addr;

	if(size < 8) {
		return addr;
	}

	reinterpret_cast<sockaddr_in *>(&addr)->sin_family =
		((uint16_t)bytes[0] << 8) + (uint16_t)bytes[1];
	memcpy(&(reinterpret_cast<sockaddr_in *>(&addr)->sin_addr), &(bytes[2]), 4);
	reinterpret_cast<sockaddr_in *>(&addr)->sin_port =
		((uint16_t)bytes[6] << 8) + (uint16_t)bytes[7];
	return addr;
}

} // namespace core
} // namespace marlin
