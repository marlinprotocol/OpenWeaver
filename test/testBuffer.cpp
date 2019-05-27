#include "gtest/gtest.h"
#include "Buffer.hpp"


using namespace marlin::net;

TEST(BufferTest, UniquePtrConstructible) {
	std::unique_ptr<char[]> uptr(new char[1400]);

	char *raw_ptr = uptr.get();

	auto buf = Buffer(std::move(uptr), 1400);

	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1400);
}

TEST(BufferTest, RawPtrConstructible) {
	char *raw_ptr = new char[1400];

	auto buf = Buffer(raw_ptr, 1400);

	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1400);
}

TEST(BufferTest, MoveConstructible) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto nbuf(std::move(buf));

	EXPECT_EQ(nbuf.data(), raw_ptr);
	EXPECT_EQ(nbuf.size(), 1400);

	EXPECT_EQ(buf.data(), nullptr);
	EXPECT_EQ(buf.size(), 0);
}

TEST(BufferTest, MoveAssignable) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	Buffer nbuf(nullptr, 0);
	nbuf = std::move(buf);

	EXPECT_EQ(nbuf.data(), raw_ptr);
	EXPECT_EQ(nbuf.size(), 1400);

	EXPECT_EQ(buf.data(), nullptr);
	EXPECT_EQ(buf.size(), 0);
}

TEST(BufferTest, CanCoverWithoutOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	bool res = buf.cover(10);

	EXPECT_TRUE(res);
	EXPECT_EQ(buf.data(), raw_ptr + 10);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferTest, CannotCoverWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	buf.cover(10);

	bool res = buf.cover(1391);

	EXPECT_FALSE(res);
	EXPECT_EQ(buf.data(), raw_ptr + 10);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferTest, CanUncoverWithoutUnderflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	buf.cover(10);

	bool res = buf.uncover(10);

	EXPECT_TRUE(res);
	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1400);
}

TEST(BufferTest, CannotUncoverWithUnderflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	buf.cover(10);

	bool res = buf.uncover(11);

	EXPECT_FALSE(res);
	EXPECT_EQ(buf.data(), raw_ptr + 10);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferTest, CanExtractUint8WithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 16;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.extract_uint8(10);

	EXPECT_EQ(num, 0x10);
}

TEST(BufferTest, CannotExtractUint8WithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.extract_uint8(1400);

	EXPECT_EQ(num, uint8_t(-1));
}

TEST(BufferTest, CanExtractUint16WithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 16;
	raw_ptr[11] = 16;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.extract_uint16(10);

	EXPECT_EQ(num, 0x1010);
}

TEST(BufferTest, CannotExtractUint16WithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.extract_uint16(1399);

	EXPECT_EQ(num, uint16_t(-1));
}

TEST(BufferTest, CanExtractUint32WithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 16;
	raw_ptr[11] = 16;
	raw_ptr[12] = 16;
	raw_ptr[13] = 16;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.extract_uint32(10);

	EXPECT_EQ(num, 0x10101010);
}

TEST(BufferTest, CannotExtractUint32WithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.extract_uint32(1397);

	EXPECT_EQ(num, uint32_t(-1));
}

TEST(BufferTest, CanExtractUint64WithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 16;
	raw_ptr[11] = 16;
	raw_ptr[12] = 16;
	raw_ptr[13] = 16;
	raw_ptr[14] = 16;
	raw_ptr[15] = 16;
	raw_ptr[16] = 16;
	raw_ptr[17] = 16;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.extract_uint64(10);

	EXPECT_EQ(num, 0x1010101010101010);
}

TEST(BufferTest, CannotExtractUint64WithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.extract_uint64(1393);

	EXPECT_EQ(num, uint64_t(-1));
}
