#ifndef MARLIN_CORE_FIBERS_MAPPINGFIBER_HPP
#define MARLIN_CORE_FIBERS_MAPPINGFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>

#include <absl/container/flat_hash_map.h>

#include <spdlog/spdlog.h>

namespace marlin {
namespace core {


template<typename ExtFabric, template<typename> typename FiberTemplate, typename Key, typename InitTuple = std::tuple<>>
class MappingFiber : public FiberScaffold<
	MappingFiber<ExtFabric, FiberTemplate, Key, InitTuple>,
	ExtFabric,
	Buffer,
	Buffer
> {
public:
	using SelfType = MappingFiber<ExtFabric, FiberTemplate, Key, InitTuple>;
	using FiberScaffoldType = FiberScaffold<
		SelfType,
		ExtFabric,
		Buffer,
		Buffer
	>;

	using typename FiberScaffoldType::InnerMessageType;
	using typename FiberScaffoldType::OuterMessageType;

	template<uint32_t tag>
	auto outer_call(auto&&, Key key, auto&&... args) {
		auto& fiber = *(fibers.try_emplace(key, new FiberTemplate<Shuttle>(std::tuple(Shuttle{*this}, init_tuple))).first->second);

		return fiber.template outer_call<tag>(*this, std::forward<decltype(args)>(args)..., key);
	}

	template<uint32_t tag>
	auto inner_call(auto&&, Key key, auto&&... args) {
		auto& fiber = *(fibers.try_emplace(key, new FiberTemplate<Shuttle>(std::tuple(Shuttle{*this}, init_tuple))).first->second);

		return fiber.template inner_call<tag>(*this, std::forward<decltype(args)>(args)..., key);
	}

	template<uint32_t tag>
	auto outer_call(auto&&, auto&& payload, Key key, auto&&... args) {
		auto& fiber = *(fibers.try_emplace(key, new FiberTemplate<Shuttle>(std::tuple(Shuttle{*this}, init_tuple))).first->second);

		return fiber.template outer_call<tag>(*this, std::forward<decltype(payload)>(payload), std::forward<decltype(args)>(args)..., key);
	}

	template<uint32_t tag>
	auto inner_call(auto&&, auto&& payload, Key key, auto&&... args) {
		auto& fiber = *(fibers.try_emplace(key, new FiberTemplate<Shuttle>(std::tuple(Shuttle{*this}, init_tuple))).first->second);

		return fiber.template inner_call<tag>(*this, std::forward<decltype(payload)>(payload), std::forward<decltype(args)>(args)..., key);
	}

	MappingFiber(auto&& init_tuple) :
		FiberScaffoldType(std::forward_as_tuple(std::get<0>(init_tuple))),
		init_tuple(std::get<1>(init_tuple)){}

private:
	struct Shuttle {
		SelfType& fabric;

		Shuttle(SelfType& fabric) : fabric(fabric) {}

		template<uint32_t tag>
		auto outer_call(auto&&, auto&&... args) {
			return fabric.ext_fabric.template outer_call<tag>(fabric, std::forward<decltype(args)>(args)...);
		}

		template<uint32_t tag>
		auto inner_call(auto&&, auto&&... args) {
			return fabric.ext_fabric.template inner_call<tag>(fabric, std::forward<decltype(args)>(args)...);
		}
	};
	absl::flat_hash_map<Key, std::unique_ptr<FiberTemplate<Shuttle>>> fibers;
	[[no_unique_address]] InitTuple init_tuple;
};


template<template<typename> typename FT, typename Key, typename IT = std::tuple<>>
struct MappingFiberF {
	template<typename T>
	using type = MappingFiber<T, FT, Key, IT>;
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_MAPPINGFIBER_HPP

