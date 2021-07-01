#include <gtest/gtest.h>
#include "marlin/core/fabric/Fabric.hpp"

#include <cstring>

using namespace marlin::core;

// namespace marlin {
// namespace core {

// }
// }

struct Terminal {
	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = false;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	// template<typename... Args>
	// Terminal(Args&&... args) {}

	int idx;
	std::vector <int> *indices;

	Terminal(std::tuple <std::tuple<int, std::vector <int>*>> &&init_tuple) :
		idx(std::get<0>(std::get<0> (init_tuple))),
		indices(std::get<1>(std::get<0>(init_tuple))) {}

	template<typename FiberType>
	int did_recv(FiberType&, Buffer&&) {
		(*indices).push_back(-1);
		return 0;
	}

	template<typename FiberType>
	int send(FiberType&, Buffer&&, SocketAddress) {
		return 0;
	}
};

template<typename ExtFabric>
struct Fiber {
	static constexpr bool is_outer_open = true;
	static constexpr bool is_inner_open = true;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	[[no_unique_address]] ExtFabric ext_fabric;
	int idx = 0;

	std::vector <int> *indices;

	template<typename ExtTupleType>
	Fiber(std::tuple<ExtTupleType, std::tuple<int, std::vector<int>*>>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		idx(std::get<0>(std::get<1>(init_tuple))),
		indices(std::get<1>(std::get<1>(init_tuple))) 
		{}

	template<typename ExtTupleType>
	Fiber(std::tuple<ExtTupleType, int>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		idx(std::get<1>(init_tuple)) {}

	int did_recv(Buffer&& buf) {
		(*indices).push_back(idx);
		// SPDLOG_INFO("Did recv: {}", idx);
		return ext_fabric.did_recv(*this, std::move(buf));
	}

	int send(InnerMessageType &&buf, SocketAddress addr) {
		(*indices).push_back(idx);
		return ext_fabric.send(*this, std::move(buf), addr);
	}

	int dial(SocketAddress, auto&& ...) {
		// (*indices).push_back(idx);
		return 0;
	}

	int bind(SocketAddress) {
		// (*indices).push_back(idx);
		return 0;
	}

	int listen() {
		// (*indices).push_back(idx);
		return 0;
	}

};

template<typename ExtFabric>
struct FiberOuterClose {
	static constexpr bool is_outer_open = false;
	static constexpr bool is_inner_open = true;

	using InnerMessageType = Buffer;
	using OuterMessageType = Buffer;

	[[no_unique_address]] ExtFabric ext_fabric;
	int idx = 0;
	std::vector <int> *indices;

	template<typename ExtTupleType>
	FiberOuterClose(std::tuple<ExtTupleType, std::tuple<int, std::vector<int>*>>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		idx(std::get<0>(std::get<1>(init_tuple))),
		indices(std::get<1>(std::get<1>(init_tuple))) 
		{}

	template<typename ExtTupleType>
	FiberOuterClose(std::tuple<ExtTupleType, int>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		idx(std::get<1>(init_tuple)) {}

	int dial(SocketAddress, auto&& ...) {
		(*indices).push_back(idx);
		return 0;
	}

	int bind(SocketAddress) {
		(*indices).push_back(idx);
		return 0;
	}

	int listen() {
		(*indices).push_back(idx);
		return 0;
	}
};



TEST(FabricTest, MessageOrder1) {
	std::vector <int> *indices = new std::vector <int>();
	Fabric <
		Terminal, 
		Fiber
	> f(std::make_tuple(
		std::make_tuple(std::make_tuple(-1, indices)),
		std::make_tuple(std::make_tuple(1, indices))
	));
	f.did_recv(Buffer(5));
	EXPECT_EQ(*indices, std::vector <int> ({1, -1}));
}

TEST(FabricTest, MessageOrder2) {
	std::vector <int> *indices = new std::vector <int>();
	Fabric<
		Terminal,
		Fiber,
		Fiber,
		Fiber,
		Fiber,
		Fiber
	> f(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(-1, indices)),
		// Other fibers
		std::make_tuple(std::make_tuple(1, indices)),
		std::make_tuple(std::make_tuple(2, indices)),
		std::make_tuple(std::make_tuple(3, indices)),
		std::make_tuple(std::make_tuple(4, indices)),
		std::make_tuple(std::make_tuple(5, indices))
	));
	f.did_recv(Buffer(5));
	EXPECT_EQ(*indices, std::vector<int>({1, 2, 3, 4, 5, -1}));
}

TEST(FabricTest, MessageOrder3) {
	std::vector <int> *indices = new std::vector <int>();
	Fabric<	
		Terminal,
		FabricF<Fiber, Fiber>::type
	> f(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(-1, indices)),
		// Other fibers
		std::make_tuple(std::make_tuple(
			std::make_tuple(1, indices)
		), std::make_tuple(
			std::make_tuple(2, indices)
		))
	));
	f.did_recv(Buffer(5));
	EXPECT_EQ(*indices, std::vector <int> ({1, 2, -1}));
}

TEST(FabricTest, MessageOrder4) {
	std::vector <int> *indices = new std::vector <int> ();
	Fabric<
		Terminal,
		Fiber,
		Fiber,
		Fiber,
		Fiber,
		FabricF<Fiber, Fiber>::type
	> f(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(-1, indices)),
		// Other fibers
		std::make_tuple(std::make_tuple(1, indices)),
		std::make_tuple(std::make_tuple(2, indices)),
		std::make_tuple(std::make_tuple(3, indices)),
		std::make_tuple(std::make_tuple(4, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices)))
	));
	f.did_recv(Buffer(5));
	EXPECT_EQ(*indices, std::vector <int> ({1, 2, 3, 4, 1, 2, -1}));
}

TEST(FabricTest, MessageOrder5) {
	std::vector <int> *indices = new std::vector <int> ();
	Fabric<
		Terminal,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber
	> f(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(-1, indices)),
		// Other fibers
		std::make_tuple(std::make_tuple(1, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(2, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(3, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(4, indices))
	));
	f.did_recv(Buffer(5)); 
	EXPECT_EQ(*indices, std::vector <int> ({1, 1, 2, 2, 1, 2, 3, 1, 2, 4, -1}));
}

TEST(FabricTest, sendFunction) {
	std::vector <int> *indices = new std::vector <int> ();
	Fabric<
		Terminal,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber
	> f(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(-1, indices)),
		// Other fibers
		std::make_tuple(std::make_tuple(1, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(2, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(3, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(4, indices))
	));
	f.send(Buffer(5), SocketAddress::from_string("0.0.0.0:3000"));
	EXPECT_EQ(*indices, std::vector <int> ({4, 2, 1, 3, 2, 1, 2, 2, 1, 1}));
}

TEST(FabricTest, dialFunction) {
	std::vector <int> *indices = new std::vector <int> ();
	Fabric<	
		Terminal,
		FiberOuterClose,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber
	> f(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(-1, indices)),
		// Other fibers
		std::make_tuple(std::make_tuple(1, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(2, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(3, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(4, indices))
	));
	f.dial(SocketAddress::from_string("0.0.0.0:3000"), Buffer(5));
	EXPECT_EQ(*indices, std::vector <int> ({1}));
}


TEST(FabricTest, bindFunction) {
	std::vector <int> *indices = new std::vector <int> ();
	Fabric<	
		Terminal,
		FiberOuterClose,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber
	> f(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(-1, indices)),
		// Other fibers
		std::make_tuple(std::make_tuple(1, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(2, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(3, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(4, indices))
	));
	f.dial(SocketAddress::from_string("0.0.0.0:3000"));
	EXPECT_EQ(*indices, std::vector <int> ({1}));

}

TEST(FabricTest, listenFunction) {
	std::vector <int> *indices = new std::vector <int> ();
	Fabric<	
		Terminal,
		FiberOuterClose,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber,
		FabricF<Fiber, Fiber>::type,
		Fiber
	> f(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(-1, indices)),
		// Other fibers
		std::make_tuple(std::make_tuple(1, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(2, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(3, indices)),
		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
						std::make_tuple(std::make_tuple(2, indices))),
		std::make_tuple(std::make_tuple(4, indices))
	));
	f.listen();
	EXPECT_EQ(*indices, std::vector <int> ({1}));
}
