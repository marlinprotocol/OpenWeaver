#include <gtest/gtest.h>

#include <spdlog/spdlog.h>
#include <marlin/lpf/StoreThenForwardBuffer.hpp>

#include <string>
#include <vector>

using namespace marlin::core;
using namespace marlin::lpf;

namespace {

struct Delegate {
	std::vector<std::string> messages;
	// Value returned for the next messages
	int res = 0;

	int did_recv_stf_message(uint16_t, Buffer&& message) {
		messages.push_back(std::string((char*)message.data(), message.size()));
		return res;
	}
};

// 8 byte big endian length followed by the message
std::string frame(std::string const& message) {
	std::string f(8, '\0');
	uint64_t length = message.size();
	for(size_t i = 0; i < 8; i++) {
		f[7 - i] = (char)(length >> (8 * i));
	}
	return f + message;
}

int recv(StoreThenForwardBuffer& buf, Delegate& delegate, std::string const& bytes) {
	Buffer b(bytes.size());
	b.write_unsafe(0, (uint8_t const*)bytes.data(), bytes.size());
	return buf.did_recv(delegate, std::move(b));
}

}

TEST(StoreThenForwardBuffer, MessagesInOneRead) {
	StoreThenForwardBuffer buf;
	Delegate delegate;

	EXPECT_EQ(recv(buf, delegate, frame("hello") + frame("") + frame("world!")), 0);
	EXPECT_EQ(delegate.messages, (std::vector<std::string>{"hello", "", "world!"}));
}

TEST(StoreThenForwardBuffer, MessageAcrossReads) {
	auto data = frame("hello") + frame(std::string(1000, 'x')) + frame("world!");

	// Every way to cut the data in two, and byte by byte
	for(size_t cut = 0; cut <= data.size(); cut++) {
		StoreThenForwardBuffer buf;
		Delegate delegate;

		EXPECT_EQ(recv(buf, delegate, data.substr(0, cut)), 0);
		EXPECT_EQ(recv(buf, delegate, data.substr(cut)), 0);
		ASSERT_EQ(delegate.messages, (std::vector<std::string>{"hello", std::string(1000, 'x'), "world!"})) << "cut at " << cut;
	}

	StoreThenForwardBuffer buf;
	Delegate delegate;
	for(auto c : data) {
		EXPECT_EQ(recv(buf, delegate, std::string(1, c)), 0);
	}
	EXPECT_EQ(delegate.messages, (std::vector<std::string>{"hello", std::string(1000, 'x'), "world!"}));
}

TEST(StoreThenForwardBuffer, RejectsBigMessage) {
	StoreThenForwardBuffer buf;
	Delegate delegate;

	// 5000001 bytes
	std::string big("\x00\x00\x00\x00\x00\x4c\x4b\x41", 8);
	EXPECT_EQ(recv(buf, delegate, big + "abc"), -1);
	EXPECT_TRUE(delegate.messages.empty());

	// Length split over two reads
	EXPECT_EQ(recv(buf, delegate, std::string("\xff\xff\xff", 3)), 0);
	EXPECT_EQ(recv(buf, delegate, std::string("\xff\xff\xff\xff\xff\xff\xff", 7)), -1);
	EXPECT_TRUE(delegate.messages.empty());

	// Usable again
	EXPECT_EQ(recv(buf, delegate, frame("hello")), 0);
	EXPECT_EQ(delegate.messages, (std::vector<std::string>{"hello"}));
}

TEST(StoreThenForwardBuffer, CleanStateAfterDelegateError) {
	StoreThenForwardBuffer buf;
	Delegate delegate;

	// The message arrives in two reads so that the buffer has seen more than 8 bytes of it
	auto data = frame(std::string(100, 'x'));
	delegate.res = -1;
	EXPECT_EQ(recv(buf, delegate, data.substr(0, 60)), 0);
	EXPECT_EQ(recv(buf, delegate, data.substr(60) + frame("dropped")), -2);
	EXPECT_EQ(delegate.messages.size(), 1u);

	// Starts with a new length. With the old state left over this read ran far past the bytes given
	delegate.res = 0;
	EXPECT_EQ(recv(buf, delegate, frame("hello").substr(0, 3)), 0);
	EXPECT_EQ(recv(buf, delegate, frame("hello").substr(3)), 0);
	EXPECT_EQ(delegate.messages, (std::vector<std::string>{std::string(100, 'x'), "hello"}));
}
