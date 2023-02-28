#ifndef MARLIN_CORE_FIBERS_MAPPINGFIBER_HPP
#define MARLIN_CORE_FIBERS_MAPPINGFIBER_HPP

#include <marlin/core/Buffer.hpp>
#include <marlin/core/fibers/FiberScaffold.hpp>

#include <absl/container/flat_hash_map.h>

#include <spdlog/spdlog.h>

namespace marlin {
namespace core {

template<typename ExtFabric, template<typename> typename FiberTemplate, typename Key>
class MappingFiber : public FiberScaffold<
	MappingFiber<ExtFabric, FiberTemplate, Key>,
	ExtFabric,
	Buffer,
	Buffer
> {
public:
	using SelfType = MappingFiber<ExtFabric, FiberTemplate, Key>;
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
		auto& fiber = *(fibers.try_emplace(key, new FiberTemplate<Shuttle>(std::tuple(Shuttle{*this}))).first->second);

		return fiber.template outer_call<tag>(*this, std::forward<decltype(args)>(args)..., key);
	}

	template<uint32_t tag>
	auto inner_call(auto&&, Key key, auto&&... args) {
		auto& fiber = *(fibers.try_emplace(key, new FiberTemplate<Shuttle>(std::tuple(Shuttle{*this}))).first->second);

		return fiber.template inner_call<tag>(*this, std::forward<decltype(args)>(args)..., key);
	}

	MappingFiber(auto&& init_tuple) :
		FiberScaffoldType(std::forward_as_tuple(std::get<0>(init_tuple))) {}

private:
	struct Shuttle {
		SelfType& fabric;

		Shuttle(SelfType& fabric) : fabric(fabric) {}

		template<uint32_t tag>
		auto outer_call(auto&&, Key key, auto&&... args) {
			return fabric.ext_fabric.template outer_call<tag>(fabric, std::forward<decltype(args)>(args)..., key);
		}

		template<uint32_t tag>
		auto inner_call(auto&&, Key key, auto&&... args) {
			return fabric.ext_fabric.template inner_call<tag>(fabric, std::forward<decltype(args)>(args)..., key);
		}
	};
	absl::flat_hash_map<Key, std::unique_ptr<FiberTemplate<Shuttle>>> fibers;
};


template<template<typename> typename FT, typename Key>
struct MappingFiberF {
	template<typename T>
	using type = MappingFiber<T, FT, Key>;
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FIBERS_MAPPINGFIBER_HPP

