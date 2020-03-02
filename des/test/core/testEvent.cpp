#include <gtest/gtest.h>

#include "marlin/simulator/core/Event.hpp"

#include <set>


using namespace marlin::simulator;

struct Stub final : public Event<Stub> {
	using Event<Stub>::Event;
	void run(Stub&) {}
};


TEST(EventConstruct, TickConstructible) {
	Stub event(10);

	EXPECT_EQ(event.get_tick(), 10);
}

TEST(EventConstruct, HasUniqueIds) {
	std::set<uint64_t> ids;
	constexpr size_t size = 1000;

	for(size_t i = 0; i < size; i++) {
		Stub event(i);
		ids.insert(event.get_id());
	}

	EXPECT_EQ(ids.size(), size);
}
