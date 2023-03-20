#include <gtest/gtest.h>
#include "marlin/core/fabric/Fabric.hpp"
#include <marlin/core/fibers/FiberScaffold.hpp>

#include <cstring>
#include <string>


using namespace marlin::core;


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

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "did_recv"_tag) {
			return did_recv(std::forward<decltype(args)>(args)...);
		} else {
			// static_assert(false) always breaks compilation
			// making it depend on a template parameter fixes it
			static_assert(tag < 0);
		}
	}

	template<uint32_t tag>
	auto inner_call(auto&&... args) {
		if constexpr (tag == "send"_tag) {
			return send(std::forward<decltype(args)>(args)...);
		} else {
			// static_assert(false) always breaks compilation
			// making it depend on a template parameter fixes it
			static_assert(tag < 0);
		}
	}

private:
	int did_recv(auto&&, Buffer&&) {
		(*indices).push_back("t_dr");
		return 0;
	}

	int send(auto&&, Buffer&&, SocketAddress) {
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

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "did_recv"_tag) {
			return did_recv(std::forward<decltype(args)>(args)...);
		} else {
			// static_assert(false) always breaks compilation
			// making it depend on a template parameter fixes it
			static_assert(tag < 0);
		}
	}

	template<uint32_t tag>
	auto inner_call(auto&&... args) {
		if constexpr (tag == "send"_tag) {
			return send(std::forward<decltype(args)>(args)...);
		} else {
			// static_assert(false) always breaks compilation
			// making it depend on a template parameter fixes it
			static_assert(tag < 0);
		}
	}

private:
	int did_recv(auto&&, Buffer&& buf) {
		(*indices).push_back("f_dr_" + std::to_string(idx));
		return ext_fabric.template outer_call<"did_recv"_tag>(*this, std::move(buf));
	}

	int send(auto&&, InnerMessageType &&buf, SocketAddress addr) {
		(*indices).push_back("f_s_" + std::to_string(idx));
		return ext_fabric.template inner_call<"send"_tag>(*this, std::move(buf), addr);
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

	template<uint32_t tag>
	auto outer_call(auto&&... args) {
		if constexpr (tag == "dial"_tag) {
			return dial(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "bind"_tag) {
			return bind(std::forward<decltype(args)>(args)...);
		} else if constexpr (tag == "listen"_tag) {
			return listen(std::forward<decltype(args)>(args)...);
		} else {
			// static_assert(false) always breaks compilation
			// making it depend on a template parameter fixes it
			static_assert(tag < 0);
		}
	}

	template<uint32_t tag>
	auto inner_call(auto&&... args) {
		if constexpr (tag == "send"_tag) {
			return send(std::forward<decltype(args)>(args)...);
		} else {
			// static_assert(false) always breaks compilation
			// making it depend on a template parameter fixes it
			static_assert(tag < 0);
		}
	}

private:
	int dial(auto&&, SocketAddress, auto&& ...) {
		(*indices).push_back("foc_d_" + std::to_string(idx));
		return 0;
	}

	int bind(auto&&, SocketAddress) {
		(*indices).push_back("foc_b_" + std::to_string(idx));
		return 0;
	}

	int listen(auto&&) {
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
	f.template outer_call<"did_recv"_tag>(0, Buffer(5));
	EXPECT_EQ(*indices, std::vector <std::string> ({"f_dr_1", "t_dr"}));

	delete indices;
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
	f.template outer_call<"did_recv"_tag>(0, Buffer(5));
	EXPECT_EQ(*indices, std::vector <std::string>({"f_dr_1", "f_dr_2", "f_dr_3", "f_dr_4", "f_dr_5", "t_dr"}));

	delete indices;
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
	f.template outer_call<"did_recv"_tag>(0, Buffer(5));
	EXPECT_EQ(*indices, std::vector <std::string> ({"f_dr_1", "f_dr_2", "t_dr"}));

	delete indices;
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
	f.template outer_call<"did_recv"_tag>(0, Buffer(5));
	EXPECT_EQ(*indices, std::vector <std::string> ({"f_dr_1", "f_dr_2", "f_dr_3", "f_dr_4", "f_dr_1", "f_dr_2", "t_dr"}));

	delete indices;
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
	f.template outer_call<"did_recv"_tag>(0, Buffer(5));
	EXPECT_EQ(*indices, std::vector <std::string> ({"f_dr_1", "f_dr_1", "f_dr_2", "f_dr_2", "f_dr_1", "f_dr_2", "f_dr_3", "f_dr_1", "f_dr_2", "f_dr_4", "t_dr"}));

	delete indices;
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
	f.template inner_call<"send"_tag>(0, Buffer(5), SocketAddress::from_string("0.0.0.0:3000"));
	EXPECT_EQ(*indices, std::vector <std::string> ({"f_s_4", "f_s_2", "f_s_1", "f_s_3", "f_s_2", "f_s_1", "f_s_2", "f_s_2", "f_s_1", "f_s_1", "t_s"}));

	delete indices;
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
	f.template outer_call<"dial"_tag>(0, SocketAddress::from_string("0.0.0.0:3000"), Buffer(5));
	EXPECT_EQ(*indices, std::vector <std::string> ({"foc_d_1"}));

	delete indices;
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
	f.template outer_call<"bind"_tag>(0, SocketAddress::from_string("0.0.0.0:3000"));
	EXPECT_EQ(*indices, std::vector <std::string> ({"foc_b_1"}));

	delete indices;
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
	f.template outer_call<"listen"_tag>(0);
	EXPECT_EQ(*indices, std::vector <std::string> ({"foc_l_1"}));

	delete indices;
}
