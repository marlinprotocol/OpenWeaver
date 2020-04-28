#ifndef MARLIN_BEACON_MESSAGES_HPP
#define MARLIN_BEACON_MESSAGES_HPP

#include <marlin/core/Buffer.hpp>


namespace marlin {
namespace beacon {

struct DISCPROTO : public core::Buffer {
public:
	DISCPROTO() : core::Buffer({0, 0}, 2) {}
};

} // namespace beacon
} // namespace marlin

#endif // MARLIN_BEACON_MESSAGES_HPP
