#ifndef MARLIN_CORE_ENDIAN_HPP
#define MARLIN_CORE_ENDIAN_HPP

// Define endianness constants
#define MARLIN_CORE_BIG_ENDIAN 1234
#define MARLIN_CORE_LITTLE_ENDIAN 4321

// Check if already defined externally
#ifndef MARLIN_CORE_ENDIANNESS

// Try endian.h
#if __has_include(<endian.h>)

#include <endian.h>
// Test BE
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
#define MARLIN_CORE_ENDIANNESS MARLIN_CORE_BIG_ENDIAN
// Test LE
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN
#define MARLIN_CORE_ENDIANNESS MARLIN_CORE_LITTLE_ENDIAN
#endif

#elif __has_include(<machine/endian.h>)

#include <machine/endian.h>
// Test BE
#if defined(__DARWIN_BYTE_ORDER) && __DARWIN_BYTE_ORDER == __DARWIN_BIG_ENDIAN
#define MARLIN_CORE_ENDIANNESS MARLIN_CORE_BIG_ENDIAN
// Test LE
#elif defined(__DARWIN_BYTE_ORDER) && __DARWIN_BYTE_ORDER == __DARWIN_LITTLE_ENDIAN
#define MARLIN_CORE_ENDIANNESS MARLIN_CORE_LITTLE_ENDIAN
#endif

#endif

// Error if still not detected
#ifndef MARLIN_CORE_ENDIANNESS
#error Could not detect endianness
#endif

#endif

#endif // MARLIN_CORE_ENDIAN_HPP
