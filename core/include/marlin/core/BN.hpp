/*! \file BN.hpp
*/

#ifndef MARLIN_CORE_BN_HPP
#define MARLIN_CORE_BN_HPP

#include <stdint.h>
#include <x86intrin.h>
#include "marlin/core/Endian.hpp"


namespace marlin {
namespace core {

struct __attribute__((packed)) alignas(32) uint256_t {
private:
#if MARLIN_CORE_ENDIANNESS == MARLIN_CORE_BIG_ENDIAN
	uint64_t hi;
	uint64_t hilo;
	uint64_t lohi;
	uint64_t lo;
#else
	uint64_t lo;
	uint64_t lohi;
	uint64_t hilo;
	uint64_t hi;
#endif
public:
	uint256_t() = default;
	uint256_t(uint256_t const& other) = default;

	uint256_t(
		uint64_t const& lo,
		uint64_t const& lohi = 0,
		uint64_t const& hilo = 0,
		uint64_t const& hi = 0
	);

	uint256_t operator+(uint256_t const& other) const;
	uint256_t& operator+=(uint256_t const& other);
	uint256_t operator-(uint256_t const& other) const;
	uint256_t& operator-=(uint256_t const& other);
	bool operator==(uint256_t const& other) const;
	bool operator<(uint256_t const& other) const;
};

} // namespace core
} // namespace marlin

#endif // MARLIN_CORE_BN_HPP
