#ifndef MARLIN_STREAM_CONNSMFIBER_HPP
#define MARLIN_STREAM_CONNSMFIBER_HPP

#include <marlin/core/fibers/FiberScaffold.hpp>
#include <spdlog/spdlog.h>

namespace marlin {
namespace stream {

template<typename ExtFabric>
class ConnSMFiber : public core::FiberScaffold<
    ConnSMFiber<ExtFabric>,
    ExtFabric,
    core::Buffer,
    core::Buffer
> {
public:
    using SelfType = ConnSMFiber<ExtFabric>;
    using FiberScaffoldType = core::FiberScaffold<
        SelfType,
        ExtFabric,
        core::Buffer,
        core::Buffer
    >;
    
    using typename FiberScaffoldType::InnerMessageType;
    using typename FiberScaffoldType::OuterMessageType;

    int testfn() {
        return 2;
    }

    ConnSMFiber(auto&&... args) : 
        FiberScaffoldType(std::forward<decltype(args)>(args)...) {
        SPDLOG_INFO("INIT");
    }

    int did_recv(auto&&, InnerMessageType&&) {
        return 9;
    }
};

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_CONNSMFIBER_HPP