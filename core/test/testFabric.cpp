#include <gtest/gtest.h>
#include "marlin/core/fabric/Fabric.hpp"

#include <cstring>
#include <string>

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
	std::vector <std::string> *indices;

	Terminal(std::tuple <std::tuple<int, std::vector <std::string>*>> &&init_tuple) :
		idx(std::get<0>(std::get<0> (init_tuple))),
		indices(std::get<1>(std::get<0>(init_tuple))) {}

	template<typename FiberType>
	int did_recv(FiberType&, Buffer&&) {
		(*indices).push_back("t_dr");
		return 0;
	}

	// int did_dial(Buffer&& ...) {
	// 	(*indices).push_back("t_dd");
	// 	return 0;
	// }


	template<typename FiberType>
	int send(FiberType&, Buffer&&, SocketAddress) {
		(*indices).push_back("t_s");
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

	std::vector <std::string> *indices;

	template<typename ExtTupleType>
	Fiber(std::tuple<ExtTupleType, std::tuple<int, std::vector <std::string>*>>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		idx(std::get<0>(std::get<1>(init_tuple))),
		indices(std::get<1>(std::get<1>(init_tuple))) 
		{}

	template<typename ExtTupleType>
	Fiber(std::tuple<ExtTupleType, int>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		idx(std::get<1>(init_tuple)) {}

	int did_recv(Buffer&& buf) {
		(*indices).push_back("f_dr_" + std::to_string(idx));
		// SPDLOG_INFO("Did recv: {}", idx);
		return ext_fabric.did_recv(*this, std::move(buf));
	}

	int send(InnerMessageType &&buf, SocketAddress addr) {
		(*indices).push_back("f_s_" + std::to_string(idx));
		return ext_fabric.send(*this, std::move(buf), addr);
	}

	int dial(SocketAddress, auto&& ...) {
		return 0;
	}

	template <typename... Args>
	int did_dial(Args&&... args) {
		(*indices).push_back("f_dd_" + std::to_string(idx));
		return ext_fabric.did_dial(*this, std::forward<Args>(args)...);
	}

	int bind(SocketAddress) {
		return 0;
	}

	int listen() {
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
	std::vector <std::string> *indices;

	template<typename ExtTupleType>
	FiberOuterClose(std::tuple<ExtTupleType, std::tuple<int, std::vector <std::string>*>>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		idx(std::get<0>(std::get<1>(init_tuple))),
		indices(std::get<1>(std::get<1>(init_tuple))) 
		{}

	template<typename ExtTupleType>
	FiberOuterClose(std::tuple<ExtTupleType, int>&& init_tuple) :
		ext_fabric(std::get<0>(init_tuple)),
		idx(std::get<1>(init_tuple)) {}


	template <typename... Args>
	int did_dial(Args&&... args) {
		(*indices).push_back("f_dd_" + std::to_string(idx));
		return ext_fabric.did_dial(*this, std::forward<Args>(args)...);
	}


	template <typename... Args>
	int dial(SocketAddress, Args&& ...) {
		(*indices).push_back("foc_d_" + std::to_string(idx));
		// return ext_fabric.did_dial(*this, std::forward<Args>(args)...);
		return 0;
	}

	int bind(SocketAddress) {
		(*indices).push_back("foc_b_" + std::to_string(idx));
		return 0;
	}

	int listen() {
		(*indices).push_back("foc_l_" + std::to_string(idx));
		return 0;
	}
};



TEST(FabricTest, MessageOrder1) {
	std::vector <std::string> *indices = new std::vector <std::string>();
	Fabric <
		Terminal, 
		Fiber
	> f(std::make_tuple(
		std::make_tuple(std::make_tuple(-1, indices)),
		std::make_tuple(std::make_tuple(1, indices))
	));
	f.did_recv(Buffer(5));
	EXPECT_EQ(*indices, std::vector <std::string> ({"f_dr_1", "t_dr"}));
}

TEST(FabricTest, MessageOrder2) {
	std::vector <std::string> *indices = new std::vector <std::string>();
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
	EXPECT_EQ(*indices, std::vector <std::string>({"f_dr_1", "f_dr_2", "f_dr_3", "f_dr_4", "f_dr_5", "t_dr"}));
}

TEST(FabricTest, MessageOrder3) {
	std::vector <std::string> *indices = new std::vector <std::string>();
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
	EXPECT_EQ(*indices, std::vector <std::string> ({"f_dr_1", "f_dr_2", "t_dr"}));
}

TEST(FabricTest, MessageOrder4) {
	std::vector <std::string> *indices = new std::vector <std::string> ();
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
	EXPECT_EQ(*indices, std::vector <std::string> ({"f_dr_1", "f_dr_2", "f_dr_3", "f_dr_4", "f_dr_1", "f_dr_2", "t_dr"}));
}

TEST(FabricTest, MessageOrder5) {
	std::vector <std::string> *indices = new std::vector <std::string> ();
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
	EXPECT_EQ(*indices, std::vector <std::string> ({"f_dr_1", "f_dr_1", "f_dr_2", "f_dr_2", "f_dr_1", "f_dr_2", "f_dr_3", "f_dr_1", "f_dr_2", "f_dr_4", "t_dr"}));
}

TEST(FabricTest, sendFunction) {
	std::vector <std::string> *indices = new std::vector <std::string> ();
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
	EXPECT_EQ(*indices, std::vector <std::string> ({"f_s_4", "f_s_2", "f_s_1", "f_s_3", "f_s_2", "f_s_1", "f_s_2", "f_s_2", "f_s_1", "f_s_1", "t_s"}));
}

TEST(FabricTest, dialFunction) {
	std::vector <std::string> *indices = new std::vector <std::string> ();
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
	EXPECT_EQ(*indices, std::vector <std::string> ({"foc_d_1"}));
}


TEST(FabricTest, bindFunction) {
	std::vector <std::string> *indices = new std::vector <std::string> ();
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
	f.bind(SocketAddress::from_string("0.0.0.0:3000"));
	EXPECT_EQ(*indices, std::vector <std::string> ({"foc_b_1"}));
}

TEST(FabricTest, listenFunction) {
	std::vector <std::string> *indices = new std::vector <std::string> ();
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
	EXPECT_EQ(*indices, std::vector <std::string> ({"foc_l_1"}));
}

// TEST(FabricTest, didDialFunction) {
// 	std::vector <std::string> *indices = new std::vector <std::string> ();
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
// 		std::make_tuple(std::make_tuple(-1, indices)),
// 		// Other fibers
// 		std::make_tuple(std::make_tuple(1, indices)),
// 		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
// 						std::make_tuple(std::make_tuple(2, indices))),
// 		std::make_tuple(std::make_tuple(2, indices)),
// 		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
// 						std::make_tuple(std::make_tuple(2, indices))),
// 		std::make_tuple(std::make_tuple(3, indices)),
// 		std::make_tuple(std::make_tuple(std::make_tuple(1, indices)), 
// 						std::make_tuple(std::make_tuple(2, indices))),
// 		std::make_tuple(std::make_tuple(4, indices))
// 	));
// 	f.did_dial(Buffer(5), SocketAddress::from_string("0.0.0.0:3000"));
// 	EXPECT_EQ(*indices, std::vector <std::string> ({"f_dr_4", "f_dr_2", "f_dr_1", "f_dr_3", "f_dr_2", "f_dr_1", "f_dr_2", "f_dr_2", "f_dr_1", "t_dr"}));

// }