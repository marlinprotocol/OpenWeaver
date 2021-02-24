#include <gtest/gtest.h>

#include "marlin/simulator/core/IndexedStorage.hpp"


using namespace marlin::simulator;

struct Stub {
	uint64_t id;

	Stub(uint64_t id) : id(id) {}

	uint64_t get_id() {
		return id;
	}
};


TEST(MapIndexConstruct, DefaultConstructible) {
	MapIndex<Stub, uint64_t, &Stub::get_id> idx;
}

TEST(MapIndexContainer, CanAddAndRetrieve) {
	MapIndex<Stub, uint64_t, &Stub::get_id> idx;
	constexpr uint64_t size = 1000;

	Stub* orig[size];

	for(uint64_t i = 0; i < size; i++) {
		orig[i] = new Stub(i);
		idx.add(orig[i]);
	}

	for(uint64_t i = 0; i < size; i++) {
		EXPECT_EQ(orig[i], idx.at(i));
		delete orig[i];
	}
}

TEST(MapIndexContainer, CanRemoveByElement) {
	MapIndex<Stub, uint64_t, &Stub::get_id> idx;
	constexpr uint64_t size = 1000;

	Stub* orig[size];

	for(uint64_t i = 0; i < size; i++) {
		orig[i] = new Stub(i);
		idx.add(orig[i]);
	}

	for(uint64_t i = 0; i < size; i++) {
		idx.remove(orig[i]);
		try {
			idx.at(i);
			FAIL();
		} catch(std::out_of_range&) {

		} catch(...) {
			FAIL();
		}
		delete orig[i];
	}

	EXPECT_TRUE(idx.is_empty());
}

TEST(MapIndexContainer, CanRemoveById) {
	MapIndex<Stub, uint64_t, &Stub::get_id> idx;
	constexpr uint64_t size = 1000;

	Stub* orig[size];

	for(uint64_t i = 0; i < size; i++) {
		orig[i] = new Stub(i);
		idx.add(orig[i]);
	}

	for(uint64_t i = 0; i < size; i++) {
		idx.remove(i);
		try {
			idx.at(i);
			FAIL();
		} catch(std::out_of_range&) {

		} catch(...) {
			FAIL();
		}
		delete orig[i];
	}

	EXPECT_TRUE(idx.is_empty());
}


TEST(MultimapIndexConstruct, DefaultConstructible) {
	MultimapIndex<Stub, uint64_t, &Stub::get_id> idx;
}

TEST(MultimapIndexContainer, CanAddAndRetrieve) {
	MultimapIndex<Stub, uint64_t, &Stub::get_id> idx;
	constexpr uint64_t size = 1000;

	Stub* orig[size];

	for(uint64_t i = 0; i < size; i++) {
		orig[size-i-1] = new Stub(size-i-1);
		idx.add(orig[size-i-1]);
		EXPECT_EQ(orig[size-i-1], idx.front());
	}

	for(uint64_t i = 0; i < size; i++) {
		delete orig[i];
	}
}

TEST(MultimapIndexContainer, CanRemoveByElement) {
	MultimapIndex<Stub, uint64_t, &Stub::get_id> idx;
	constexpr uint64_t size = 1000;

	Stub* orig[size];

	for(uint64_t i = 0; i < size; i++) {
		orig[size-i-1] = new Stub(size-i-1);
		idx.add(orig[size-i-1]);
	}

	for(uint64_t i = 0; i < size-1; i++) {
		idx.remove(orig[i]);
		EXPECT_NE(idx.front(), orig[i]);
		delete orig[i];
	}

	idx.remove(orig[size - 1]);
	delete orig[size - 1];

	EXPECT_TRUE(idx.is_empty());
}
