#include "gtest/gtest.h"
#include <marlin/net/Buffer.hpp>

#include <cstring>

using namespace marlin::net;

TEST(BufferConstruct, UniquePtrConstructible) {
	std::unique_ptr<char[]> uptr(new char[1400]);

	char *raw_ptr = uptr.get();

	auto buf = Buffer(std::move(uptr), 1400);

	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1400);
}

TEST(BufferConstruct, RawPtrConstructible) {
	char *raw_ptr = new char[1400];

	auto buf = Buffer(raw_ptr, 1400);

	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1400);
}

TEST(BufferConstruct, MoveConstructible) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto nbuf(std::move(buf));

	EXPECT_EQ(nbuf.data(), raw_ptr);
	EXPECT_EQ(nbuf.size(), 1400);

	EXPECT_EQ(buf.data(), nullptr);
	EXPECT_EQ(buf.size(), 0);
}

TEST(BufferConstruct, MoveAssignable) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	Buffer nbuf(nullptr, 0);
	nbuf = std::move(buf);

	EXPECT_EQ(nbuf.data(), raw_ptr);
	EXPECT_EQ(nbuf.size(), 1400);

	EXPECT_EQ(buf.data(), nullptr);
	EXPECT_EQ(buf.size(), 0);
}

TEST(BufferResize, CanCoverWithoutOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	bool res = buf.cover(10);

	EXPECT_TRUE(res);
	EXPECT_EQ(buf.data(), raw_ptr + 10);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferResize, CannotCoverWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	buf.cover(10);

	bool res = buf.cover(1391);

	EXPECT_FALSE(res);
	EXPECT_EQ(buf.data(), raw_ptr + 10);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferResize, CanUncoverWithoutUnderflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	buf.cover(10);

	bool res = buf.uncover(10);

	EXPECT_TRUE(res);
	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1400);
}

TEST(BufferResize, CannotUncoverWithUnderflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	buf.cover(10);

	bool res = buf.uncover(11);

	EXPECT_FALSE(res);
	EXPECT_EQ(buf.data(), raw_ptr + 10);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferResize, CanTruncateWithoutOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	bool res = buf.truncate(10);

	EXPECT_TRUE(res);
	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferResize, CannotTruncateWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	buf.truncate(10);

	bool res = buf.truncate(1391);

	EXPECT_FALSE(res);
	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferResize, CanExpandWithoutUnderflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	buf.truncate(10);

	bool res = buf.expand(10);

	EXPECT_TRUE(res);
	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1400);
}

TEST(BufferResize, CannotExpandWithUnderflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	buf.truncate(10);

	bool res = buf.expand(11);

	EXPECT_FALSE(res);
	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferRead, CanReadUint8WithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 1;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint8(10);

	EXPECT_EQ(num, 0x01);
}

TEST(BufferRead, CannotReadUint8WithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint8(1400);

	EXPECT_EQ(num, uint8_t(-1));
}

TEST(BufferRead, CanReadUint16WithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint16(10);

	EXPECT_TRUE(std::memcmp(&num, raw_ptr + 10, 2) == 0);
}

TEST(BufferRead, CannotReadUint16WithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint16(1399);

	EXPECT_EQ(num, uint16_t(-1));
}

TEST(BufferReadLE, CanReadUint16LEWithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint16_le(10);

	EXPECT_EQ(num, 0x0201);
}

TEST(BufferReadLE, CannotReadUint16LEWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint16_le(1399);

	EXPECT_EQ(num, uint16_t(-1));
}

TEST(BufferReadBE, CanReadUint16BEWithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint16_be(10);

	EXPECT_EQ(num, 0x0102);
}

TEST(BufferReadBE, CannotReadUint16BEWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint16_be(1399);

	EXPECT_EQ(num, uint16_t(-1));
}

TEST(BufferRead, CanReadUint32WithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	raw_ptr[12] = 3;
	raw_ptr[13] = 4;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint32(10);

	EXPECT_TRUE(std::memcmp(&num, raw_ptr + 10, 4) == 0);
}

TEST(BufferRead, CannotReadUint32WithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint32(1397);

	EXPECT_EQ(num, uint32_t(-1));
}

TEST(BufferReadLE, CanReadUint32LEWithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	raw_ptr[12] = 3;
	raw_ptr[13] = 4;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint32_le(10);

	EXPECT_EQ(num, 0x04030201);
}

TEST(BufferReadLE, CannotReadUint32LEWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint32_le(1397);

	EXPECT_EQ(num, uint32_t(-1));
}

TEST(BufferReadBE, CanReadUint32BEWithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	raw_ptr[12] = 3;
	raw_ptr[13] = 4;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint32_be(10);

	EXPECT_EQ(num, 0x01020304);
}

TEST(BufferReadBE, CannotReadUint32BEWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint32_be(1397);

	EXPECT_EQ(num, uint32_t(-1));
}

TEST(BufferRead, CanReadUint64WithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	raw_ptr[12] = 3;
	raw_ptr[13] = 4;
	raw_ptr[14] = 5;
	raw_ptr[15] = 6;
	raw_ptr[16] = 7;
	raw_ptr[17] = 8;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint64(10);

	EXPECT_TRUE(std::memcmp(&num, raw_ptr + 10, 8) == 0);
}

TEST(BufferRead, CannotReadUint64WithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint64(1393);

	EXPECT_EQ(num, uint64_t(-1));
}

TEST(BufferReadLE, CanReadUint64LEWithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	raw_ptr[12] = 3;
	raw_ptr[13] = 4;
	raw_ptr[14] = 5;
	raw_ptr[15] = 6;
	raw_ptr[16] = 7;
	raw_ptr[17] = 8;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint64_le(10);

	EXPECT_EQ(num, 0x0807060504030201);
}

TEST(BufferReadLE, CannotReadUint64LEWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint64_le(1393);

	EXPECT_EQ(num, uint64_t(-1));
}

TEST(BufferReadBE, CanReadUint64BEWithoutOverflow) {
	char *raw_ptr = new char[1400];
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	raw_ptr[12] = 3;
	raw_ptr[13] = 4;
	raw_ptr[14] = 5;
	raw_ptr[15] = 6;
	raw_ptr[16] = 7;
	raw_ptr[17] = 8;
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint64_be(10);

	EXPECT_EQ(num, 0x0102030405060708);
}

TEST(BufferReadBE, CannotReadUint64BEWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	auto num = buf.read_uint64_be(1393);

	EXPECT_EQ(num, uint64_t(-1));
}

TEST(BufferWrite, CanWriteUint8WithoutOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	uint8_t num = 0x01;

	auto res = buf.write_uint8(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, &num, 1) == 0);
}

TEST(BufferWrite, CannotWriteUint8WithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	bool res = buf.write_uint8(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWrite, CanWriteUint16WithoutOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	uint16_t num = 0x0102;

	auto res = buf.write_uint16(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, &num, 2) == 0);
}

TEST(BufferWrite, CannotWriteUint16WithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	bool res = buf.write_uint16(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWriteLE, CanWriteUint16LEWithoutOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	uint16_t num = 0x0102;

	auto res = buf.write_uint16_le(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, "\x02\x01", 2) == 0);
}

TEST(BufferWriteLE, CannotWriteUint16LEWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	bool res = buf.write_uint16_le(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWriteBE, CanWriteUint16BEWithoutOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	uint16_t num = 0x0102;

	auto res = buf.write_uint16_be(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, "\x01\x02", 2) == 0);
}

TEST(BufferWriteBE, CannotWriteUint16BEWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	bool res = buf.write_uint16_be(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWrite, CanWriteUint32WithoutOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	uint32_t num = 0x01020304;

	auto res = buf.write_uint32(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, &num, 4) == 0);
}

TEST(BufferWrite, CannotWriteUint32WithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	bool res = buf.write_uint32(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWriteLE, CanWriteUint32LEWithoutOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	uint32_t num = 0x01020304;

	auto res = buf.write_uint32_le(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, "\x04\x03\x02\x01", 4) == 0);
}

TEST(BufferWriteLE, CannotWriteUint32LEWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	bool res = buf.write_uint32_le(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWriteBE, CanWriteUint32BEWithoutOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	uint32_t num = 0x01020304;

	auto res = buf.write_uint32_be(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, "\x01\x02\x03\x04", 4) == 0);
}

TEST(BufferWriteBE, CannotWriteUint32BEWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	bool res = buf.write_uint32_be(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWrite, CanWriteUint64WithoutOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	uint64_t num = 0x0102030405060708;

	auto res = buf.write_uint64(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, &num, 8) == 0);
}

TEST(BufferWrite, CannotWriteUint64WithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	bool res = buf.write_uint64(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWriteLE, CanWriteUint64LEWithoutOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	uint64_t num = 0x0102030405060708;

	auto res = buf.write_uint64_le(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, "\x08\x07\x06\x05\x04\x03\x02\x01", 8) == 0);
}

TEST(BufferWriteLE, CannotWriteUint64LEWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	bool res = buf.write_uint64_le(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWriteBE, CanWriteUint64BEWithoutOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);
	uint64_t num = 0x0102030405060708;

	auto res = buf.write_uint64_be(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, "\x01\x02\x03\x04\x05\x06\x07\x08", 8) == 0);
}

TEST(BufferWriteBE, CannotWriteUint64BEWithOverflow) {
	char *raw_ptr = new char[1400];
	auto buf = Buffer(raw_ptr, 1400);

	bool res = buf.write_uint64_be(1400, 0);

	EXPECT_FALSE(res);
}
