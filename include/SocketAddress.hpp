#ifndef MARLIN_NET_SOCKETADDRESS_HPP
#define MARLIN_NET_SOCKETADDRESS_HPP

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <vector>

namespace marlin {
namespace net {

/// Wraps sockaddr(_*) and adds convenience methods
class SocketAddress: public sockaddr_storage
{
public:
	/// Initialize zero address by default
	SocketAddress();

	/// Copy from another SocketAddress
	SocketAddress(const SocketAddress &addr);
	/// Copy assign from another SocketAddress
	SocketAddress &operator=(const SocketAddress &addr);

	/// Copy from a sockaddr_storage
	SocketAddress(const sockaddr_storage &addr);
	/// Copy assign from a sockaddr_storage
	SocketAddress &operator=(const sockaddr_storage &addr);

	/// Copy from a sockaddr
	SocketAddress(const sockaddr &addr);
	/// Copy assign from a sockaddr
	SocketAddress &operator=(const sockaddr &addr);

	/// Copy from a sockaddr_in
	SocketAddress(const sockaddr_in &addr);
	/// Copy assign from a sockaddr_in
	SocketAddress &operator=(const sockaddr_in &addr);

	/// Copy from a sockaddr_in6
	SocketAddress(const sockaddr_in6 &addr);
	/// Copy assign from a sockaddr_in6
	SocketAddress &operator=(const sockaddr_in6 &addr);

	/// Build from address string
	static SocketAddress from_string(std::string addrString);
	/// Return the address and port in standard notation
	std::string to_string() const;

	/// Loopback IPv4 address
	static SocketAddress loopback_ipv4(const uint16_t port);

	/// Equality operator
	bool operator==(const SocketAddress &other) const;

	/// Comparison operator
	bool operator<(const SocketAddress &other) const;

	/// Serialize into bytes
	std::vector<unsigned char> serialize() const;
	/// Deserialize from bytes
	static SocketAddress deserialize(const std::vector<unsigned char>::iterator bytes);
};

} // namespace net
} // namespace marlin

#endif // MARLIN_NET_SOCKETADDRESS_HPP
