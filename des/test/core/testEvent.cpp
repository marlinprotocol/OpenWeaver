#include <gtest/gtest.h>

#include "marlin/simulator/core/Event.hpp"

#include <set>


using namespace marlin::simulator;

struct Stub final : public Event<Stub> {
	using Event<Stub>::Event;

	std::function<void(Stub&)> run_impl;

	void run(Stub& stub) {
		run_impl(stub);
	}
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

TEST(EventConstruct, CallsRunOfDerivedClass) {
	Stub stub(10);
	Event<Stub>& event = *(Event<Stub>*)&stub;

	bool did_call = false;

	stub.run_impl = [&](Stub& s) {
		EXPECT_EQ(&s, &stub);
		did_call = true;
	};

	event.run(stub);

	EXPECT_TRUE(did_call);
}
