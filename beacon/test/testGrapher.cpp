#include <gtest/gtest.h>

#include <marlin/beacon/ClusterDiscoverer.hpp>

#include "../../asyncio/test/HttpTestServer.hpp"

#include <map>
#include <string>

using namespace marlin::beacon;

namespace {

std::string const key1 = "0x8f2f8a5d3d7a2e9a2d1c2d7a1a2b3c4d5e6f7a8b";
std::string const id1 = "0x00000000000000000000000000000000000000a1";
std::string const key2 = "0xABCDEFabcdef0123456789abcdefABCDEF012345";
std::string const id2 = "0x00000000000000000000000000000000000000b2";

std::string reply(std::string const& clusters) {
	return "{\"data\":{\"clusters\":[" + clusters + "]}}";
}

std::string cluster(std::string const& client_key, std::string const& id) {
	return "{\"clientKey\":\"" + client_key + "\",\"id\":\"" + id + "\"}";
}

}

TEST(GrapherAddress, AcceptsAddress) {
	EXPECT_TRUE(is_hex_address(key1));
	EXPECT_TRUE(is_hex_address(key2));
}

TEST(GrapherAddress, RejectsWrongLength) {
	EXPECT_FALSE(is_hex_address(""));
	EXPECT_FALSE(is_hex_address("0x"));
	EXPECT_FALSE(is_hex_address(key1.substr(0, 41)));
	EXPECT_FALSE(is_hex_address(key1.substr(0, 40)));
	EXPECT_FALSE(is_hex_address(key1 + "0"));
	EXPECT_FALSE(is_hex_address(key1 + "00"));
	EXPECT_FALSE(is_hex_address(key1 + std::string(1000, 'a')));
}

TEST(GrapherAddress, RejectsWrongPrefix) {
	EXPECT_FALSE(is_hex_address("0X" + key1.substr(2)));
	EXPECT_FALSE(is_hex_address("00" + key1.substr(2)));
	EXPECT_FALSE(is_hex_address(key1.substr(2) + "0x"));
}

TEST(GrapherAddress, RejectsNonHex) {
	auto s = key1;
	s[41] = 'g';
	EXPECT_FALSE(is_hex_address(s));
	s = key1;
	s[2] = ' ';
	EXPECT_FALSE(is_hex_address(s));
	s = key1;
	s[20] = '\0';
	EXPECT_FALSE(is_hex_address(s));
	s = key1;
	s[10] = '-';
	EXPECT_FALSE(is_hex_address(s));
}

TEST(GrapherParse, ReadsClusters) {
	std::map<std::string, std::string> m;
	EXPECT_TRUE(Grapher::parse_clusters(reply(cluster(key1, id1) + "," + cluster(key2, id2)), m));

	EXPECT_EQ(m.size(), 2u);
	EXPECT_EQ(m[key1], id1);
	EXPECT_EQ(m[key2], id2);
}

TEST(GrapherParse, EmptyClusters) {
	std::map<std::string, std::string> m;
	EXPECT_TRUE(Grapher::parse_clusters(reply(""), m));
	EXPECT_TRUE(m.empty());
}

TEST(GrapherParse, RejectsUnexpectedShape) {
	std::map<std::string, std::string> m;
	EXPECT_FALSE(Grapher::parse_clusters("", m));
	EXPECT_FALSE(Grapher::parse_clusters("EOF", m));
	EXPECT_FALSE(Grapher::parse_clusters("[]", m));
	EXPECT_FALSE(Grapher::parse_clusters("{\"errors\":[{\"message\":\"auth error: API key not found\"}]}", m));
	EXPECT_FALSE(Grapher::parse_clusters("{\"data\":null}", m));
	EXPECT_FALSE(Grapher::parse_clusters("{\"data\":{\"clusters\":{}}}", m));
	EXPECT_TRUE(m.empty());
}

TEST(GrapherParse, SkipsMalformedEntries) {
	std::map<std::string, std::string> m;
	auto body = reply(
		// id longer than 20 bytes
		cluster(key1, id1 + "ffffffffffffffffffffffffffffffffffffffff") + ","
		// id shorter than 20 bytes
		+ cluster(key1, "0x00") + ","
		// id not hex
		+ cluster(key1, "0xzz000000000000000000000000000000000000a1") + ","
		// id without prefix
		+ cluster(key1, id1.substr(2) + "00") + ","
		// client key too long
		+ cluster(key1 + "00", id1) + ","
		// client key not hex
		+ cluster("hello", id1) + ","
		// wrong types
		+ "{\"clientKey\":1,\"id\":2},"
		+ "{\"clientKey\":\"" + key1 + "\"},"
		+ "null,"
		+ "\"" + key1 + "\","
		// the only good entry
		+ cluster(key2, id2)
	);
	EXPECT_TRUE(Grapher::parse_clusters(body, m));

	EXPECT_EQ(m.size(), 1u);
	EXPECT_EQ(m[key2], id2);
}

TEST(GrapherHexToInt, DecodesAddress) {
	auto res = hextoint<20>(id1);

	std::array<uint8_t, 20> expected = {};
	expected[19] = 0xa1;
	EXPECT_EQ(res, expected);
}

TEST(GrapherHexToInt, DoesNotWritePastTheArray) {
	// 2 guard bytes after the 2 byte result must be left alone
	struct {
		std::array<uint8_t, 2> res;
		std::array<uint8_t, 2> guard;
	} s;
	s.guard = {0x55, 0x55};
	s.res = hextoint<2>("0x0102030405060708090a0b0c0d0e0f");

	EXPECT_EQ(s.res, (std::array<uint8_t, 2>{1, 2}));
	EXPECT_EQ(s.guard, (std::array<uint8_t, 2>{0x55, 0x55}));
}

TEST(GrapherHexToInt, ShortAndOddStrings) {
	EXPECT_EQ(hextoint<2>(""), (std::array<uint8_t, 2>{0, 0}));
	EXPECT_EQ(hextoint<2>("0x"), (std::array<uint8_t, 2>{0, 0}));
	EXPECT_EQ(hextoint<2>("0xab"), (std::array<uint8_t, 2>{0xab, 0}));
	EXPECT_EQ(hextoint<2>("0xabc"), (std::array<uint8_t, 2>{0xab, 0}));
}

TEST(GrapherQuery, KeepsLastGoodClusters) {
	TestServer server("200 OK", 0);
	server.set_reply("200 OK", reply(cluster(key1, id1)));
	marlin::asyncio::subgraph_url_override() = server.url();

	{
		Grapher g;
		int updates = 0;
		g.on_update = [&]() { updates++; };

		ASSERT_TRUE(run_loop_until([&]() { return updates == 1; }));
		EXPECT_EQ(g.clientkey_id_map.size(), 1u);
		EXPECT_EQ(g.clientkey_id_map[key1], id1);
		EXPECT_EQ(g.failures, 0u);

		// Well formed reply without clusters
		server.set_reply("200 OK", reply(""));
		g.query();
		ASSERT_TRUE(run_loop_until([&]() { return g.failures == 1; }));
		EXPECT_EQ(g.clientkey_id_map.size(), 1u);

		// HTTP error
		server.set_reply("500 Internal Server Error", "EOF");
		g.query();
		ASSERT_TRUE(run_loop_until([&]() { return g.failures == 2; }));
		EXPECT_EQ(g.clientkey_id_map.size(), 1u);

		// Not a subgraph reply
		server.set_reply("200 OK", "{\"errors\":[{\"message\":\"auth error: API key not found\"}]}");
		g.query();
		ASSERT_TRUE(run_loop_until([&]() { return g.failures == 3; }));
		EXPECT_EQ(g.clientkey_id_map.size(), 1u);
		EXPECT_EQ(g.clientkey_id_map[key1], id1);
		EXPECT_EQ(updates, 1);

		// Good reply again
		server.set_reply("200 OK", reply(cluster(key2, id2)));
		g.query();
		ASSERT_TRUE(run_loop_until([&]() { return updates == 2; }));
		EXPECT_EQ(g.failures, 0u);
		EXPECT_EQ(g.clientkey_id_map.size(), 1u);
		EXPECT_EQ(g.clientkey_id_map[key2], id2);
	}

	// Closes the timer of the Grapher
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
	marlin::asyncio::subgraph_url_override() = "";
}

TEST(GrapherQuery, DestroyedWhileQuerying) {
	// The reply arrives 300 ms after the request, the Grapher is gone by then
	TestServer server("200 OK", 0, true, 300);
	server.set_reply("200 OK", reply(cluster(key1, id1)));
	marlin::asyncio::subgraph_url_override() = server.url();

	auto* updates = new int(0);
	{
		Grapher g;
		g.on_update = [updates]() { (*updates)++; };
		ASSERT_TRUE(run_loop_until([&]() { return server.num_requests == 1; }));
	}
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);

	EXPECT_EQ(*updates, 0);
	delete updates;
	marlin::asyncio::subgraph_url_override() = "";
}
