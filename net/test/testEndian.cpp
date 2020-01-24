#include "gtest/gtest.h"
#include <marlin/net/Endian.hpp>

#include <arpa/inet.h>

TEST(EndianTest, SetsCorrectEndianness) {
	if(htons(1) == 1) {
		// BE
		EXPECT_EQ(MARLIN_NET_ENDIANNESS, MARLIN_NET_BIG_ENDIAN);
	} else {
		// LE
		EXPECT_EQ(MARLIN_NET_ENDIANNESS, MARLIN_NET_LITTLE_ENDIAN);
	}
}
