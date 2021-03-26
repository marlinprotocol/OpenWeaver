#ifndef MARLIN_CORE_FABRIC_FABRIC_HPP
#define MARLIN_CORE_FABRIC_FABRIC_HPP

namespace marlin {
namespace core {

template<typename... Fibers>
class Fabric {

private:
	// Important: Not zero indexed!
	std::tuple<void, Fibers...> fibers;
};

}  // namespace core
}  // namespace marlin

#endif  // MARLIN_CORE_FABRIC_FABRIC_HPP
