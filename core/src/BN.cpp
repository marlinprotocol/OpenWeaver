#include "marlin/core/BN.hpp"


namespace marlin {
namespace core {

uint256_t::uint256_t(
	uint64_t const& lo,
	uint64_t const& lohi,
	uint64_t const& hilo,
	uint64_t const& hi
) : lo(lo), lohi(lohi), hilo(hilo), hi(hi) {}

uint256_t uint256_t::operator+(uint256_t const& other) const {
	uint256_t temp;
	bool carry = false;
	carry = _addcarry_u64(carry, this->lo, other.lo, (unsigned long long*)&temp.lo);
	carry = _addcarry_u64(carry, this->lohi, other.lohi, (unsigned long long*)&temp.lohi);
	carry = _addcarry_u64(carry, this->hilo, other.hilo, (unsigned long long*)&temp.hilo);
	carry = _addcarry_u64(carry, this->hi, other.hi, (unsigned long long*)&temp.hi);
	return temp;
}

uint256_t& uint256_t::operator+=(uint256_t const& other) {
	bool carry = false;
	carry = _addcarry_u64(carry, this->lo, other.lo, (unsigned long long*)&this->lo);
	carry = _addcarry_u64(carry, this->lohi, other.lohi, (unsigned long long*)&this->lohi);
	carry = _addcarry_u64(carry, this->hilo, other.hilo, (unsigned long long*)&this->hilo);
	carry = _addcarry_u64(carry, this->hi, other.hi, (unsigned long long*)&this->hi);
	return *this;
}

uint256_t uint256_t::operator-(uint256_t const& other) const {
	uint256_t temp;
	bool carry = false;
	carry = _subborrow_u64(carry, this->lo, other.lo, (unsigned long long*)&temp.lo);
	carry = _subborrow_u64(carry, this->lohi, other.lohi, (unsigned long long*)&temp.lohi);
	carry = _subborrow_u64(carry, this->hilo, other.hilo, (unsigned long long*)&temp.hilo);
	carry = _subborrow_u64(carry, this->hi, other.hi, (unsigned long long*)&temp.hi);
	return temp;
}

uint256_t& uint256_t::operator-=(uint256_t const& other) {
	bool carry = false;
	carry = _subborrow_u64(carry, this->lo, other.lo, (unsigned long long*)&this->lo);
	carry = _subborrow_u64(carry, this->lohi, other.lohi, (unsigned long long*)&this->lohi);
	carry = _subborrow_u64(carry, this->hilo, other.hilo, (unsigned long long*)&this->hilo);
	carry = _subborrow_u64(carry, this->hi, other.hi, (unsigned long long*)&this->hi);
	return *this;
}

bool uint256_t::operator==(uint256_t const& other) const {
	return this->hi == other.hi && this->lohi == other.lohi && this->hilo == other.hilo && this->lo == other.lo;
}

bool uint256_t::operator<(uint256_t const& other) const {
	uint256_t temp;
	bool carry = false;
	carry = _subborrow_u64(carry, this->lo, other.lo, (unsigned long long*)&temp.lo);
	carry = _subborrow_u64(carry, this->lohi, other.lohi, (unsigned long long*)&temp.lohi);
	carry = _subborrow_u64(carry, this->hilo, other.hilo, (unsigned long long*)&temp.hilo);
	carry = _subborrow_u64(carry, this->hi, other.hi, (unsigned long long*)&temp.hi);
	return carry;
}

} // namespace core
} // namespace marlin
