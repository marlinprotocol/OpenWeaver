#include "gtest/gtest.h"
#include "protocol/AckRanges.hpp"


using namespace marlin::stream;

TEST(AckRangesTest, First) {
	AckRanges ranges;

	ranges.add_packet_number(10);

	EXPECT_EQ(ranges.largest, 10);
	EXPECT_EQ(ranges.ranges, std::list<uint64_t>({1}));
}

TEST(AckRangesTest, Largest) {
	AckRanges ranges;
	ranges.largest = 4;
	ranges.ranges.push_back(5);

	ranges.add_packet_number(10);

	EXPECT_EQ(ranges.largest, 10);
	EXPECT_EQ(ranges.ranges, std::list<uint64_t>({1, 5, 5}));
}

TEST(AckRangesTest, Existing) {
	AckRanges ranges;
	ranges.largest = 10;
	ranges.ranges.push_back(1);
	ranges.ranges.push_back(5);
	ranges.ranges.push_back(5);

	ranges.add_packet_number(3);

	EXPECT_EQ(ranges.largest, 10);
	EXPECT_EQ(ranges.ranges, std::list<uint64_t>({1, 5, 5}));
}

TEST(AckRangesTest, BeginningOfGap) {
	AckRanges ranges;
	ranges.largest = 10;
	ranges.ranges.push_back(1);
	ranges.ranges.push_back(5);
	ranges.ranges.push_back(5);

	ranges.add_packet_number(9);

	EXPECT_EQ(ranges.largest, 10);
	EXPECT_EQ(ranges.ranges, std::list<uint64_t>({2, 4, 5}));
}

TEST(AckRangesTest, EndOfGap) {
	AckRanges ranges;
	ranges.largest = 10;
	ranges.ranges.push_back(1);
	ranges.ranges.push_back(5);
	ranges.ranges.push_back(5);

	ranges.add_packet_number(5);

	EXPECT_EQ(ranges.largest, 10);
	EXPECT_EQ(ranges.ranges, std::list<uint64_t>({1, 4, 6}));
}

TEST(AckRangesTest, MiddleOfGap) {
	AckRanges ranges;
	ranges.largest = 10;
	ranges.ranges.push_back(1);
	ranges.ranges.push_back(5);
	ranges.ranges.push_back(5);

	ranges.add_packet_number(7);

	EXPECT_EQ(ranges.largest, 10);
	EXPECT_EQ(ranges.ranges, std::list<uint64_t>({1, 2, 1, 2, 5}));
}

TEST(AckRangesTest, FillGap) {
	AckRanges ranges;
	ranges.largest = 10;
	ranges.ranges.push_back(5);
	ranges.ranges.push_back(1);
	ranges.ranges.push_back(5);

	ranges.add_packet_number(5);

	EXPECT_EQ(ranges.largest, 10);
	EXPECT_EQ(ranges.ranges, std::list<uint64_t>({11}));
}

TEST(AckRangesTest, Last) {
	AckRanges ranges;
	ranges.largest = 10;
	ranges.ranges.push_back(5);

	ranges.add_packet_number(3);

	EXPECT_EQ(ranges.largest, 10);
	EXPECT_EQ(ranges.ranges, std::list<uint64_t>({5, 2, 1}));
}
