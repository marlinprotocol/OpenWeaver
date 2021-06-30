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

	template<typename... Args>
	Terminal(Args&&...) {}

	template<typename FiberType>
	int did_recv(FiberType&, Buffer&&) {
		// SPDLOG_INFO("Did recv: Terminal");
		return 0;
	}

	template<typename FiberType>
	int did_recv(std::vector <int> &indices, FiberType&, Buffer&&) {
		indices.push_back(-1);
		// SPDLOG_INFO("Did recv: Terminal");
		return 0;
	}

	template<typename FiberType>
	int send(std::vector <int> &, FiberType&, Buffer&&, SocketAddress) {
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

	template<typename ExtTupleType>
	Fiber(std::tuple<ExtTupleType, int>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		idx(std::get<1>(init_tuple)) {}

	int did_recv(Buffer&& buf) {
		// SPDLOG_INFO("Did recv: {}", idx);
		return ext_fabric.did_recv(*this, std::move(buf));
	}

	int did_recv(std::vector <int> &indices, Buffer&& buf) {
		// SPDLOG_INFO("Did recv: {}", idx);
		indices.push_back(idx);
		return ext_fabric.did_recv(indices, *this, std::move(buf));
	}

	int send(std::vector <int> &indices, InnerMessageType &&buf, SocketAddress addr) {
		indices.push_back(idx);
		return ext_fabric.send(indices, *this, std::move(buf), addr);
	}

	int dial(std::vector <int> &indices, SocketAddress, auto&& ...) {
		std::cout << idx << std::endl;
		indices.push_back(idx);
		return 0;
	}

	int bind(std::vector <int> &indices, SocketAddress) {
		indices.push_back(idx);
		return 0;
	}

	int listen(std::vector <int> &indices) {
		indices.push_back(idx);
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

	template<typename ExtTupleType>
	FiberOuterClose(std::tuple<ExtTupleType, int>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		idx(std::get<1>(init_tuple)) {}

	int dial(std::vector <int> &indices, SocketAddress, auto&& ...) {
		std::cout << idx << std::endl;
		indices.push_back(idx);
		return 0;
	}

	int bind(std::vector <int> &indices, SocketAddress) {
		indices.push_back(idx);
		return 0;
	}

	int listen(std::vector <int> &indices) {
		indices.push_back(idx);
		return 0;
	}
};



TEST(FabricTest, MessageOrder1) {
	std::vector <int> indices;
	Fabric <
		Terminal, 
		Fiber
	> f(std::make_tuple(
		std::make_tuple(std::make_tuple(), 0),
		std::make_tuple(1)
	));
	f.did_recv(indices, Buffer(5));
	EXPECT_EQ(indices, std::vector<int>({1, -1}));
}

TEST(FabricTest, MessageOrder2) {
	std::vector <int> indices;
	Fabric<
		Terminal,
		Fiber,
		Fiber,
		Fiber,
		Fiber,
		Fiber
	> f(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(), 0),
		// Other fibers
		std::make_tuple(1),
		std::make_tuple(2),
		std::make_tuple(3),
		std::make_tuple(4),
		std::make_tuple(5)
	));
	f.did_recv(indices, Buffer(5));
	EXPECT_EQ(indices, std::vector<int>({1, 2, 3, 4, 5, -1}));
}

TEST(FabricTest, MessageOrder3) {
	std::vector <int> indices;
	Fabric<	
		Terminal,
		FabricF<Fiber, Fiber>::type
	> f(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(), 0),
		// Other fibers
		std::make_tuple(std::make_tuple(1), std::make_tuple(2))
	));
	f.did_recv(indices, Buffer(5));
	EXPECT_EQ(indices, std::vector <int> ({1, 2, -1}));
}

TEST(FabricTest, MessageOrder4) {
	std::vector <int> indices;
	Fabric<
		Terminal,
		Fiber,
		Fiber,
		Fiber,
		Fiber,
		FabricF<Fiber, Fiber>::type
	> f(std::make_tuple(
		// Terminal
		std::make_tuple(std::make_tuple(), 0),
		// Other fibers
		std::make_tuple(1),
		std::make_tuple(2),
		std::make_tuple(3),
		std::make_tuple(4),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2))
	));
	f.did_recv(indices, Buffer(5));
	EXPECT_EQ(indices, std::vector <int> ({1, 2, 3, 4, 1, 2, -1}));
}

TEST(FabricTest, MessageOrder5) {
	std::vector <int> indices;
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
		std::make_tuple(std::make_tuple(), 0),
		// Other fibers
		std::make_tuple(1),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(2),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(3),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(4)
	));
	f.did_recv(indices, Buffer(5)); 
	EXPECT_EQ(indices, std::vector <int> ({1, 1, 2, 2, 1, 2, 3, 1, 2, 4, -1}));
}

TEST(FabricTest, sendFunction) {
	std::vector <int> indices;
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
		std::make_tuple(std::make_tuple(), 0),
		// Other fibers
		std::make_tuple(1),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(2),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(3),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(4)
	));
	f.send(indices, Buffer(5), SocketAddress::from_string("0:0:0:0:3000"));
	EXPECT_EQ(indices, std::vector <int> ({4, 2, 1, 3, 2, 1, 2, 2, 1, 1}));
}

TEST(FabricTest, dialFunction) {
	std::vector <int> indices;
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
		std::make_tuple(std::make_tuple(), 0),
		// Other fibers
		std::make_tuple(1),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(2),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(3),
		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
		std::make_tuple(4)
	));
	f.dial(indices, SocketAddress::from_string("0.0.0.0:3000"), Buffer(5));
	EXPECT_EQ(indices, std::vector <int> ({1}));
}

// TEST(FabricTest, sendFunction) {
// 	std::vector <int> indices;
// 	Fabric<	
// 		Terminal,
// 		Fiber,
// 		FabricF<Fiber, Fiber>::type,
// 		Fiber,
// 		FabricF<Fiber, Fiber>::type,
// 		Fiber,
// 		FabricF<Fiber, Fiber>::type,
// 		Fiber
// 	> f(std::make_tuple(
// 		// Terminal
// 		std::make_tuple(std::make_tuple(), 0),
// 		// Other fibers
// 		std::make_tuple(1),
// 		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
// 		std::make_tuple(2),
// 		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
// 		std::make_tuple(3),
// 		std::make_tuple(std::make_tuple(1), std::make_tuple(2)),
// 		std::make_tuple(4)
// 	));
	
// }