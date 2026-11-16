#include <gtest/gtest.h>

#include <marlin/pubsub/PubSubNode.hpp>

#include <string>
#include <tuple>
#include <vector>

using namespace marlin::pubsub;

namespace {

std::string const mpond = "0x1635815984abab0dbb9afd77984dad69c24bf3d711bc0ddb1e2d53ef2d523e5e";
std::string const pond = "0x5802add45f8ec0a524470683e7295faacc853f97cf4a8d3ffbaaf25ce0fd87c4";
std::string const id1 = "0x00000000000000000000000000000000000000a1";
std::string const id2 = "0x00000000000000000000000000000000000000B2";

using Stakes = std::unordered_map<std::array<uint8_t, 20>, uint64_t>;

std::array<uint8_t, 20> addr(uint8_t last) {
	std::array<uint8_t, 20> a = {};
	a[19] = last;
	return a;
}

std::string delegation(std::string const& token, std::string const& amount) {
	return "{\"token\":{\"tokenId\":\"" + token + "\"},\"amount\":\"" + amount + "\"}";
}

std::string cluster(std::string const& id, std::string const& delegations) {
	return "{\"id\":\"" + id + "\",\"totalDelegations\":[" + delegations + "]}";
}

std::string reply(std::string const& clusters) {
	return "{\"data\":{\"clusters\":[" + clusters + "]}}";
}

// 1 MPond and 1 POND in the units of the subgraph
std::string const one = "1000000000000000000";

}

TEST(StakeRequesterAmount, Table) {
	// amount, decimals, ok, value
	std::vector<std::tuple<std::string, size_t, bool, uint64_t>> cases = {
		{"0", 0, true, 0},
		{"7", 0, true, 7},
		{"1234567", 3, true, 1234},
		{"1234567", 7, true, 0},
		{"1234567", 12, true, 0},
		{one, 12, true, 1000000},
		{one, 18, true, 1},
		{"0001000", 0, true, 1000},
		// 19 digits are the most that is accepted
		{"9999999999999999999", 0, true, 9999999999999999999ull},
		{"9999999999999999999" + std::string(18, '0'), 18, true, 9999999999999999999ull},
		// 20 digits and more could overflow
		{"10000000000000000000", 0, false, 0},
		{"18446744073709551616", 0, false, 0},
		{std::string(20, '9') + std::string(12, '0'), 12, false, 0},
		{std::string(100000, '9'), 12, false, 0},
		// not decimal numbers
		{"", 12, false, 0},
		{"abc", 0, false, 0},
		{"abcdefghijklmnopqrstuvwxyz", 12, false, 0},
		{"-1000000000000000000", 12, false, 0},
		{"+1000000000000000000", 12, false, 0},
		{" 1000000000000000000", 12, false, 0},
		{"1000000000000000000 ", 12, false, 0},
		{"0x1000000000000000000", 12, false, 0},
		{"1e18", 0, false, 0},
		{"1.5", 0, false, 0},
		{"100000a000000000000", 12, false, 0},
		// malformed digits in the part which is dropped
		{"1000000000000000a00", 12, false, 0},
		{std::string("12\0" "34", 5), 0, false, 0},
	};

	for(auto& [amount, decimals, ok, value] : cases) {
		uint64_t out = 12345;
		EXPECT_EQ(StakeRequester::parse_amount(amount, decimals, out), ok) << amount.substr(0, 50) << " / " << decimals;
		if(ok) {
			EXPECT_EQ(out, value) << amount.substr(0, 50) << " / " << decimals;
		}
	}
}

TEST(StakeRequesterParse, ReadsStakes) {
	Stakes stakes;
	auto body = reply(
		// 1 MPond and 2500 POND
		cluster(id1, delegation(mpond, one) + "," + delegation(pond, "2500" + std::string(18, '0'))) + ","
		// 0.5 MPond
		+ cluster(id2, delegation(mpond, "500000000000000000"))
	);
	EXPECT_TRUE(StakeRequester::parse_stakes(body, stakes));

	EXPECT_EQ(stakes.size(), 2u);
	EXPECT_EQ(stakes[addr(0xa1)], 1000000u + 2500u);
	EXPECT_EQ(stakes[addr(0xb2)], 500000u);
}

TEST(StakeRequesterParse, MPondThreshold) {
	Stakes stakes;
	auto body = reply(
		// just under 0.5 MPond
		cluster(id1, delegation(mpond, "499999999999999999") + "," + delegation(pond, "2500" + std::string(18, '0'))) + ","
		// POND only
		+ cluster(id2, delegation(pond, "1000000" + std::string(18, '0')))
	);
	EXPECT_TRUE(StakeRequester::parse_stakes(body, stakes));
	EXPECT_TRUE(stakes.empty());
}

TEST(StakeRequesterParse, RejectsUnexpectedShape) {
	Stakes stakes;
	EXPECT_FALSE(StakeRequester::parse_stakes("", stakes));
	EXPECT_FALSE(StakeRequester::parse_stakes("EOF", stakes));
	EXPECT_FALSE(StakeRequester::parse_stakes("{\"errors\":[{\"message\":\"auth error: API key not found\"}]}", stakes));
	EXPECT_FALSE(StakeRequester::parse_stakes("{\"data\":{\"clusters\":null}}", stakes));
	EXPECT_TRUE(stakes.empty());

	EXPECT_TRUE(StakeRequester::parse_stakes(reply(""), stakes));
	EXPECT_TRUE(stakes.empty());
}

TEST(StakeRequesterParse, MalformedAmountsDoNotThrow) {
	Stakes stakes;
	auto body = reply(
		// not a number, std::stoull would throw std::invalid_argument
		cluster(id1, delegation(mpond, "abcdefghijklmnopqrstuvwxyz") + "," + delegation(pond, "abcdefghijklmnopqrstuvwxyz")) + ","
		// too large, std::stoull would throw std::out_of_range
		+ cluster(id1, delegation(mpond, std::string(40, '9'))) + ","
		+ cluster(id1, delegation(pond, std::string(60, '9')) + "," + delegation(mpond, "")) + ","
		// a malformed delegation is skipped, the others of the cluster still count
		+ cluster(id2, delegation(pond, "12x") + "," + delegation(mpond, one) + "," + delegation(mpond, "-" + one))
	);
	EXPECT_NO_THROW({
		EXPECT_TRUE(StakeRequester::parse_stakes(body, stakes));
	});

	EXPECT_EQ(stakes.size(), 1u);
	EXPECT_EQ(stakes[addr(0xb2)], 1000000u);
}

TEST(StakeRequesterParse, TotalSaturates) {
	Stakes stakes;
	auto big = "9999999999999999999" + std::string(12, '0');
	auto body = reply(cluster(id1, delegation(mpond, big) + "," + delegation(mpond, big)));
	EXPECT_TRUE(StakeRequester::parse_stakes(body, stakes));

	EXPECT_EQ(stakes[addr(0xa1)], UINT64_MAX);
}

TEST(StakeRequesterParse, SkipsMalformedClusters) {
	Stakes stakes;
	auto body = reply(
		// id of the wrong length
		cluster(id1 + "00", delegation(mpond, one)) + ","
		+ cluster("0x00", delegation(mpond, one)) + ","
		// wrong types
		+ "{\"id\":1,\"totalDelegations\":[]},"
		+ "{\"id\":\"" + id1 + "\",\"totalDelegations\":{}},"
		+ "{\"id\":\"" + id1 + "\",\"totalDelegations\":[1,null,{\"amount\":1},{\"amount\":\"1\",\"token\":1}]},"
		+ "null,"
		+ cluster(id2, delegation(mpond, one))
	);
	EXPECT_TRUE(StakeRequester::parse_stakes(body, stakes));

	EXPECT_EQ(stakes.size(), 1u);
	EXPECT_EQ(stakes[addr(0xb2)], 1000000u);
}
