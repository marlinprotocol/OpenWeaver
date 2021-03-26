#ifndef MARLIN_CORE_FABRIC_FABRIC_HPP
#define MARLIN_CORE_FABRIC_FABRIC_HPP

namespace marlin {
namespace core {

// Fibers assumed to be ordered from Outer to Inner
template<typename... Fibers>
class Fabric {

private:
	// Important: Not zero indexed!
	std::tuple<void, Fibers...> fibers;
public:
	using OuterMessageType = decltype(std::get<0>(stages))::OuterMessageType;
	using InnerMessageType = decltype(std::get<sizeof...(Fibers) - 1>(fibers))::InnerMessageType;
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FABRIC_FABRIC_HPP
