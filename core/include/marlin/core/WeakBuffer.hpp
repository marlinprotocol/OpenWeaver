#ifndef MARLIN_CORE_WEAKBUFFER_HPP
#define MARLIN_CORE_WEAKBUFFER_HPP

#include "marlin/core/BaseBuffer.hpp"

namespace marlin {
namespace core {

/// @brief Byte buffer implementation with modifiable bounds without memory ownership
/// @headerfile WeakBuffer.hpp <marlin/core/WeakBuffer.hpp>
class WeakBuffer : public BaseBuffer<WeakBuffer> {
public:
	using BaseBuffer<WeakBuffer>::BaseBuffer;
};

} // namespace core
} // namespace marlin

#endif // MARLIN_CORE_WEAKBUFFER_HPP
