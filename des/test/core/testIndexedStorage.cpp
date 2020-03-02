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

	std::shared_ptr<Stub> orig[size];

	for(uint64_t i = 0; i < size; i++) {
		orig[i] = std::make_shared<Stub>(i);
		idx.add(orig[i]);
	}

	for(uint64_t i = 0; i < size; i++) {
		EXPECT_EQ(orig[i], idx.at(i));
	}
}

TEST(MapIndexContainer, CanRemoveByElement) {
	MapIndex<Stub, uint64_t, &Stub::get_id> idx;
	constexpr uint64_t size = 1000;

	std::shared_ptr<Stub> orig[size];

	for(uint64_t i = 0; i < size; i++) {
		orig[i] = std::make_shared<Stub>(i);
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
	}

	EXPECT_TRUE(idx.is_empty());
}

TEST(MapIndexContainer, CanRemoveById) {
	MapIndex<Stub, uint64_t, &Stub::get_id> idx;
	constexpr uint64_t size = 1000;

	std::shared_ptr<Stub> orig[size];

	for(uint64_t i = 0; i < size; i++) {
		orig[i] = std::make_shared<Stub>(i);
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
	}

	EXPECT_TRUE(idx.is_empty());
}
