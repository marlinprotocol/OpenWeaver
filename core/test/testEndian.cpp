#include "gtest/gtest.h"
#include "marlin/core/Endian.hpp"

#include <arpa/inet.h>

TEST(EndianTest, SetsCorrectEndianness) {
	if(htons(1) == 1) {
		// BE
		EXPECT_EQ(MARLIN_CORE_ENDIANNESS, MARLIN_CORE_BIG_ENDIAN);
	} else {
		// LE
		EXPECT_EQ(MARLIN_CORE_ENDIANNESS, MARLIN_CORE_LITTLE_ENDIAN);
	}
}
