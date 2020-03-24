#include "gtest/gtest.h"
#include <marlin/net/core/BN.hpp>

#include <cstring>

using namespace marlin::net;


struct StubBN {
#if MARLIN_NET_ENDIANNESS == MARLIN_NET_BIG_ENDIAN
	int8_t direction = -1;
	uint8_t low = 31;
#else
	int8_t direction = 1;
	uint8_t low = 0;
#endif
	uint8_t mem[32] = {};

	uint8_t& operator[](uint8_t offset) {
		return mem[low+direction*offset];
	}
};

TEST(uint256Construct, uint8Constructible) {
	uint256_t n((uint8_t)0x12);

	StubBN e;
	e[0] = 0x12;

	EXPECT_TRUE(std::memcmp((char*)&n, (char*)e.mem, 32) == 0);
}

TEST(uint256Construct, uint16Constructible) {
	uint256_t n((uint16_t)0x1234);

	StubBN e;
	e[1] = 0x12;
	e[0] = 0x34;

	EXPECT_TRUE(std::memcmp((char*)&n, (char*)e.mem, 32) == 0);
}

TEST(uint256Construct, uint32Constructible) {
	uint256_t n((uint32_t)0x12345678);

	StubBN e;
	e[3] = 0x12;
	e[2] = 0x34;
	e[1] = 0x56;
	e[0] = 0x78;

	EXPECT_TRUE(std::memcmp((char*)&n, (char*)e.mem, 32) == 0);
}

TEST(uint256Construct, uint64Constructible) {
	uint256_t n((uint64_t)0x123456789abcdef0);

	StubBN e;
	e[7] = 0x12;
	e[6] = 0x34;
	e[5] = 0x56;
	e[4] = 0x78;
	e[3] = 0x9a;
	e[2] = 0xbc;
	e[1] = 0xde;
	e[0] = 0xf0;

	EXPECT_TRUE(std::memcmp((char*)&n, (char*)e.mem, 32) == 0);
}

TEST(uint256Construct, uint64x2Constructible) {
	uint256_t n(
		(uint64_t)0x123456789abcdef0,
		(uint64_t)0x123456789abcdef0
	);

	StubBN e;
	e[15] = e[7] = 0x12;
	e[14] = e[6] = 0x34;
	e[13] = e[5] = 0x56;
	e[12] = e[4] = 0x78;
	e[11] = e[3] = 0x9a;
	e[10] = e[2] = 0xbc;
	e[9] = e[1] = 0xde;
	e[8] = e[0] = 0xf0;

	EXPECT_TRUE(std::memcmp((char*)&n, (char*)e.mem, 32) == 0);
}

TEST(uint256Construct, uint64x3Constructible) {
	uint256_t n(
		(uint64_t)0x123456789abcdef0,
		(uint64_t)0x123456789abcdef0,
		(uint64_t)0x123456789abcdef0
	);

	StubBN e;
	e[23] = e[15] = e[7] = 0x12;
	e[22] = e[14] = e[6] = 0x34;
	e[21] = e[13] = e[5] = 0x56;
	e[20] = e[12] = e[4] = 0x78;
	e[19] = e[11] = e[3] = 0x9a;
	e[18] = e[10] = e[2] = 0xbc;
	e[17] = e[9] = e[1] = 0xde;
	e[16] = e[8] = e[0] = 0xf0;

	EXPECT_TRUE(std::memcmp((char*)&n, (char*)e.mem, 32) == 0);
}

TEST(uint256Construct, uint64x4Constructible) {
	uint256_t n(
		(uint64_t)0x123456789abcdef0,
		(uint64_t)0x123456789abcdef0,
		(uint64_t)0x123456789abcdef0,
		(uint64_t)0x123456789abcdef0
	);

	StubBN e;
	e[31] = e[23] = e[15] = e[7] = 0x12;
	e[30] = e[22] = e[14] = e[6] = 0x34;
	e[29] = e[21] = e[13] = e[5] = 0x56;
	e[28] = e[20] = e[12] = e[4] = 0x78;
	e[27] = e[19] = e[11] = e[3] = 0x9a;
	e[26] = e[18] = e[10] = e[2] = 0xbc;
	e[25] = e[17] = e[9] = e[1] = 0xde;
	e[24] = e[16] = e[8] = e[0] = 0xf0;

	EXPECT_TRUE(std::memcmp((char*)&n, (char*)e.mem, 32) == 0);
}
