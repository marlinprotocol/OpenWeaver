#include "gtest/gtest.h"
#include "marlin/core/Buffer.hpp"

#include <cstring>

using namespace marlin::core;

TEST(BufferConstruct, SizeConstructible) {
	auto buf = Buffer(1400);

	EXPECT_EQ(buf.size(), 1400);
}

TEST(BufferConstruct, InitializerListConstructible) {
	auto buf = Buffer({'0','1','2','3'}, 1400);

	EXPECT_EQ(buf.size(), 1400);
	EXPECT_TRUE(std::memcmp(buf.data(), "0123", 4) == 0);
}

TEST(BufferConstruct, RawPtrConstructible) {
	uint8_t *raw_ptr = new uint8_t[1400];

	auto buf = Buffer(raw_ptr, 1400);

	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1400);
}

TEST(BufferConstruct, MoveConstructible) {
	auto buf = Buffer(1400);

	uint8_t *raw_ptr = buf.data();

	auto nbuf(std::move(buf));

	EXPECT_EQ(nbuf.data(), raw_ptr);
	EXPECT_EQ(nbuf.size(), 1400);

	EXPECT_EQ(buf.data(), nullptr);
	EXPECT_EQ(buf.size(), 0);
}

TEST(BufferConstruct, MoveAssignable) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();

	Buffer nbuf(nullptr, 0);
	nbuf = std::move(buf);

	EXPECT_EQ(nbuf.data(), raw_ptr);
	EXPECT_EQ(nbuf.size(), 1400);

	EXPECT_EQ(buf.data(), nullptr);
	EXPECT_EQ(buf.size(), 0);
}

TEST(BufferResize, CanCoverWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();

	bool res = buf.cover(10);

	EXPECT_TRUE(res);
	EXPECT_EQ(buf.data(), raw_ptr + 10);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferResize, CannotCoverWithOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	buf.cover_unsafe(10);

	bool res = buf.cover(1391);

	EXPECT_FALSE(res);
	EXPECT_EQ(buf.data(), raw_ptr + 10);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferResize, CanUncoverWithoutUnderflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	buf.cover_unsafe(10);

	bool res = buf.uncover(10);

	EXPECT_TRUE(res);
	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1400);
}

TEST(BufferResize, CannotUncoverWithUnderflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	buf.cover_unsafe(10);

	bool res = buf.uncover(11);

	EXPECT_FALSE(res);
	EXPECT_EQ(buf.data(), raw_ptr + 10);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferResize, CanTruncateWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();

	bool res = buf.truncate(10);

	EXPECT_TRUE(res);
	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferResize, CannotTruncateWithOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	buf.truncate_unsafe(10);

	bool res = buf.truncate(1391);

	EXPECT_FALSE(res);
	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferResize, CanExpandWithoutUnderflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	buf.truncate_unsafe(10);

	bool res = buf.expand(10);

	EXPECT_TRUE(res);
	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1400);
}

TEST(BufferResize, CannotExpandWithUnderflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	buf.truncate_unsafe(10);

	bool res = buf.expand(11);

	EXPECT_FALSE(res);
	EXPECT_EQ(buf.data(), raw_ptr);
	EXPECT_EQ(buf.size(), 1390);
}

TEST(BufferRead, CanReadArbitraryWithoutOverflow) {
	auto buf = Buffer({'0','1','2','3','4','5','6','7'}, 1400);
	uint8_t *raw_ptr = buf.data();

	uint8_t out[3];

	auto res = buf.read(4, out, 3);

	EXPECT_EQ(res, true);
	EXPECT_TRUE(std::memcmp(out, raw_ptr + 4, 3) == 0);
}

TEST(BufferRead, CannotReadArbitraryWithOverflow) {
	auto buf = Buffer({'0','1','2','3','4','5','6','7'}, 1400);

	uint8_t out[3];

	auto res = buf.read(1398, out, 3);

	EXPECT_EQ(res, false);
}

TEST(BufferRead, CanReadUint8WithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	raw_ptr[10] = 1;

	auto num = buf.read_uint8(10);

	EXPECT_TRUE(num.has_value());
	EXPECT_EQ(num.value(), 0x01);
}

TEST(BufferRead, CannotReadUint8WithOverflow) {
	auto buf = Buffer(1400);

	auto num = buf.read_uint8(1400);

	EXPECT_FALSE(num.has_value());
}

TEST(BufferRead, CanReadUint16WithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;

	auto num = buf.read_uint16(10);

	EXPECT_TRUE(num.has_value());
	EXPECT_TRUE(std::memcmp(&num.value(), raw_ptr + 10, 2) == 0);
}

TEST(BufferRead, CannotReadUint16WithOverflow) {
	auto buf = Buffer(1400);

	auto num = buf.read_uint16(1399);

	EXPECT_FALSE(num.has_value());
}

TEST(BufferReadLE, CanReadUint16LEWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;

	auto num = buf.read_uint16_le(10);

	EXPECT_TRUE(num.has_value());
	EXPECT_EQ(num.value(), 0x0201);
}

TEST(BufferReadLE, CannotReadUint16LEWithOverflow) {
	auto buf = Buffer(1400);

	auto num = buf.read_uint16_le(1399);

	EXPECT_FALSE(num.has_value());
}

TEST(BufferReadBE, CanReadUint16BEWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;

	auto num = buf.read_uint16_be(10);

	EXPECT_TRUE(num.has_value());
	EXPECT_EQ(num.value(), 0x0102);
}

TEST(BufferReadBE, CannotReadUint16BEWithOverflow) {
	auto buf = Buffer(1400);

	auto num = buf.read_uint16_be(1399);

	EXPECT_FALSE(num.has_value());
}

TEST(BufferRead, CanReadUint32WithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	raw_ptr[12] = 3;
	raw_ptr[13] = 4;

	auto num = buf.read_uint32(10);

	EXPECT_TRUE(std::memcmp(&num, raw_ptr + 10, 4) == 0);
}

TEST(BufferRead, CannotReadUint32WithOverflow) {
	auto buf = Buffer(1400);

	auto num = buf.read_uint32(1397);

	EXPECT_EQ(num, uint32_t(-1));
}

TEST(BufferReadLE, CanReadUint32LEWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	raw_ptr[12] = 3;
	raw_ptr[13] = 4;

	auto num = buf.read_uint32_le(10);

	EXPECT_EQ(num, 0x04030201);
}

TEST(BufferReadLE, CannotReadUint32LEWithOverflow) {
	auto buf = Buffer(1400);

	auto num = buf.read_uint32_le(1397);

	EXPECT_EQ(num, uint32_t(-1));
}

TEST(BufferReadBE, CanReadUint32BEWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	raw_ptr[12] = 3;
	raw_ptr[13] = 4;

	auto num = buf.read_uint32_be(10);

	EXPECT_EQ(num, 0x01020304);
}

TEST(BufferReadBE, CannotReadUint32BEWithOverflow) {
	auto buf = Buffer(1400);

	auto num = buf.read_uint32_be(1397);

	EXPECT_EQ(num, uint32_t(-1));
}

TEST(BufferRead, CanReadUint64WithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	raw_ptr[12] = 3;
	raw_ptr[13] = 4;
	raw_ptr[14] = 5;
	raw_ptr[15] = 6;
	raw_ptr[16] = 7;
	raw_ptr[17] = 8;

	auto num = buf.read_uint64(10);

	EXPECT_TRUE(std::memcmp(&num, raw_ptr + 10, 8) == 0);
}

TEST(BufferRead, CannotReadUint64WithOverflow) {
	auto buf = Buffer(1400);

	auto num = buf.read_uint64(1393);

	EXPECT_EQ(num, uint64_t(-1));
}

TEST(BufferReadLE, CanReadUint64LEWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	raw_ptr[12] = 3;
	raw_ptr[13] = 4;
	raw_ptr[14] = 5;
	raw_ptr[15] = 6;
	raw_ptr[16] = 7;
	raw_ptr[17] = 8;

	auto num = buf.read_uint64_le(10);

	EXPECT_EQ(num, 0x0807060504030201);
}

TEST(BufferReadLE, CannotReadUint64LEWithOverflow) {
	auto buf = Buffer(1400);

	auto num = buf.read_uint64_le(1393);

	EXPECT_EQ(num, uint64_t(-1));
}

TEST(BufferReadBE, CanReadUint64BEWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	raw_ptr[10] = 1;
	raw_ptr[11] = 2;
	raw_ptr[12] = 3;
	raw_ptr[13] = 4;
	raw_ptr[14] = 5;
	raw_ptr[15] = 6;
	raw_ptr[16] = 7;
	raw_ptr[17] = 8;

	auto num = buf.read_uint64_be(10);

	EXPECT_EQ(num, 0x0102030405060708);
}

TEST(BufferReadBE, CannotReadUint64BEWithOverflow) {
	auto buf = Buffer(1400);

	auto num = buf.read_uint64_be(1393);

	EXPECT_EQ(num, uint64_t(-1));
}

TEST(BufferWrite, CanWriteArbitraryWithoutOverflow) {
	auto buf = Buffer({'0','1','2','3','4','5','6','7'}, 1400);
	uint8_t *raw_ptr = buf.data();

	uint8_t in[3] {'a', 'b', 'c'};

	auto res = buf.write(4, in, 3);

	EXPECT_EQ(res, true);
	EXPECT_TRUE(std::memcmp("0123abc7", raw_ptr, 8) == 0);
}

TEST(BufferWrite, CannotWriteArbitraryWithOverflow) {
	auto buf = Buffer({'0','1','2','3','4','5','6','7'}, 1400);
	uint8_t *raw_ptr = buf.data();

	uint8_t in[3] {'a', 'b', 'c'};

	auto res = buf.write(1398, in, 3);

	EXPECT_EQ(res, false);
	EXPECT_TRUE(std::memcmp("01234567", raw_ptr, 8) == 0);
}


TEST(BufferWrite, CanWriteUint8WithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	uint8_t num = 0x01;

	auto res = buf.write_uint8(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, &num, 1) == 0);
}

TEST(BufferWrite, CannotWriteUint8WithOverflow) {
	auto buf = Buffer(1400);

	bool res = buf.write_uint8(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWrite, CanWriteUint16WithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	uint16_t num = 0x0102;

	auto res = buf.write_uint16(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, &num, 2) == 0);
}

TEST(BufferWrite, CannotWriteUint16WithOverflow) {
	auto buf = Buffer(1400);

	bool res = buf.write_uint16(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWriteLE, CanWriteUint16LEWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	uint16_t num = 0x0102;

	auto res = buf.write_uint16_le(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, "\x02\x01", 2) == 0);
}

TEST(BufferWriteLE, CannotWriteUint16LEWithOverflow) {
	auto buf = Buffer(1400);

	bool res = buf.write_uint16_le(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWriteBE, CanWriteUint16BEWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	uint16_t num = 0x0102;

	auto res = buf.write_uint16_be(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, "\x01\x02", 2) == 0);
}

TEST(BufferWriteBE, CannotWriteUint16BEWithOverflow) {
	auto buf = Buffer(1400);

	bool res = buf.write_uint16_be(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWrite, CanWriteUint32WithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	uint32_t num = 0x01020304;

	auto res = buf.write_uint32(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, &num, 4) == 0);
}

TEST(BufferWrite, CannotWriteUint32WithOverflow) {
	auto buf = Buffer(1400);

	bool res = buf.write_uint32(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWriteLE, CanWriteUint32LEWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	uint32_t num = 0x01020304;

	auto res = buf.write_uint32_le(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, "\x04\x03\x02\x01", 4) == 0);
}

TEST(BufferWriteLE, CannotWriteUint32LEWithOverflow) {
	auto buf = Buffer(1400);

	bool res = buf.write_uint32_le(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWriteBE, CanWriteUint32BEWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	uint32_t num = 0x01020304;

	auto res = buf.write_uint32_be(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, "\x01\x02\x03\x04", 4) == 0);
}

TEST(BufferWriteBE, CannotWriteUint32BEWithOverflow) {
	auto buf = Buffer(1400);

	bool res = buf.write_uint32_be(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWrite, CanWriteUint64WithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	uint64_t num = 0x0102030405060708;

	auto res = buf.write_uint64(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, &num, 8) == 0);
}

TEST(BufferWrite, CannotWriteUint64WithOverflow) {
	auto buf = Buffer(1400);

	bool res = buf.write_uint64(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWriteLE, CanWriteUint64LEWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	uint64_t num = 0x0102030405060708;

	auto res = buf.write_uint64_le(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, "\x08\x07\x06\x05\x04\x03\x02\x01", 8) == 0);
}

TEST(BufferWriteLE, CannotWriteUint64LEWithOverflow) {
	auto buf = Buffer(1400);

	bool res = buf.write_uint64_le(1400, 0);

	EXPECT_FALSE(res);
}

TEST(BufferWriteBE, CanWriteUint64BEWithoutOverflow) {
	auto buf = Buffer(1400);
	uint8_t *raw_ptr = buf.data();
	uint64_t num = 0x0102030405060708;

	auto res = buf.write_uint64_be(10, num);

	EXPECT_TRUE(res);
	EXPECT_TRUE(std::memcmp(raw_ptr + 10, "\x01\x02\x03\x04\x05\x06\x07\x08", 8) == 0);
}

TEST(BufferWriteBE, CannotWriteUint64BEWithOverflow) {
	auto buf = Buffer(1400);

	bool res = buf.write_uint64_be(1400, 0);

	EXPECT_FALSE(res);
}
