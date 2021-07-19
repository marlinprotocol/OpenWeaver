#ifndef MARLIN_CORE_FIBERS_DYNAMICFRAMINGFIBER_HPP
#define MARLIN_CORE_FIBERS_DYNAMICFRAMINGFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>

#include <variant>

#include <spdlog/spdlog.h>

namespace marlin {
namespace core {

template<typename ExtFabric, template<typename> typename... FiberTemplates>
class DynamicFramingFiber : public FiberScaffold<
	DynamicFramingFiber<ExtFabric, FiberTemplates...>,
	ExtFabric,
	Buffer,
	Buffer
> {
public:
	using SelfType = DynamicFramingFiber<ExtFabric, FiberTemplates...>;
	using FiberScaffoldType = FiberScaffold<
		SelfType,
		ExtFabric,
		Buffer,
		Buffer
	>;

	using typename FiberScaffoldType::InnerMessageType;
	using typename FiberScaffoldType::OuterMessageType;

	// todo: explore fabric like static designs instead of references
	std::variant<FiberTemplates<SelfType&>...> fibers;

	DynamicFramingFiber(auto&& init_tuple) :
		FiberScaffoldType(std::forward_as_tuple(std::get<0>(init_tuple))),
		fibers(std::in_place_index_t<0>(), std::tuple_cat(std::forward_as_tuple(*this), std::get<1>(init_tuple))) {}

	void reset(uint64_t max_len) {
		return std::visit([&](auto&& fiber) {
			return fiber.o(*this).reset(max_len);
		}, fibers);
	}

	int leftover(auto&&, InnerMessageType&& bytes, SocketAddress addr) {
		return std::visit([&](auto&& fiber) {
			return fiber.i(*this).did_recv(*this, std::move(bytes), addr);
		}, fibers);
	}

	template<size_t idx>
	auto& transform(auto&&... args) {
		SPDLOG_INFO("Transform: {}", idx);
		return fibers.template emplace<idx>(std::forward_as_tuple(*this, args...));
	}

	int did_recv(auto&& src, InnerMessageType&& bytes, SocketAddress addr) {
		bool self = std::visit([&](auto&& fiber) {
			if constexpr (std::is_same_v<decltype(src), decltype(fiber)>)
				return &fiber == &src;
			else return false;
		}, fibers);

		if(self) {
			// escape
			return FiberScaffoldType::did_recv(*this, std::move(bytes), addr);
		}

		return std::visit([&](auto&& fiber) {
			return fiber.i(*this).did_recv(*this, std::move(bytes), addr);
		}, fibers);
	}
};


template<template<typename> typename... FT>
struct DynamicFramingFiberHelper {
	template<typename T>
	using type = DynamicFramingFiber<T, FT...>;
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_DYNAMICFRAMINGFIBER_HPP
