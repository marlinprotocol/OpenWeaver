#ifndef MARLIN_CORE_CIDRBLOCK_HPP
#define MARLIN_CORE_CIDRBLOCK_HPP

#include <string>
#include <vector>

#include "marlin/core/SocketAddress.hpp"

namespace marlin {
namespace core {

/// SocketAddress subclass which uses port to store prefix length
class CidrBlock : public SocketAddress
{
public:
	/// Initialize 0.0.0.0/0 by default
	CidrBlock();
	/// Copy from another CidrBlock
	CidrBlock(const CidrBlock &addr);

	/// Build from block string
	static CidrBlock from_string(std::string blockString);
	/// Return the block in standard notation
	std::string to_string() const;

	/// Returns true if the block contains addr
	bool does_contain_address(const SocketAddress &addr) const;
};

} // namespace core
} // namespace marlin

#endif // MARLIN_CORE_CIDRBLOCK_HPP
