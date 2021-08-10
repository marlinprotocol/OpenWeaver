#ifndef MARLIN_STREAM_CONNSMFIBER_HPP
#define MARLIN_STREAM_CONNSMFIBER_HPP

#include <marlin/core/fibers/FiberScaffold.hpp>
#include <marlin/core/messages/BaseMessage.hpp>
#include <spdlog/spdlog.h>

using namespace marlin;

namespace marlin {
namespace stream {

enum struct ConnectionState {
        Listen,
        DialSent,
        DialRcvd,
        Established,
        Closing
};

/* Cannot be preceded by VersioningFiber */
template<typename ExtFabric>
class ConnSMFiber : public core::FiberScaffold<
    ConnSMFiber<ExtFabric>,
    ExtFabric*,
    core::Buffer,
    core::Buffer
> {
public:
    using SelfType = ConnSMFiber<ExtFabric>;
    using FiberScaffoldType = core::FiberScaffold<
        SelfType,
        ExtFabric*,
        core::Buffer,
        core::Buffer
    >;
    using BaseMessageType = marlin::core::BaseMessage;
    
    using typename FiberScaffoldType::InnerMessageType;
    using typename FiberScaffoldType::OuterMessageType;

private:
    /// List of possible connection states
    ConnectionState conn_state = ConnectionState::Listen;
    uint8_t streamver;

    /// Did we initiate the connection by dialling?
    bool dialled = false;

public:
    ConnSMFiber(std::tuple<ExtFabric*, uint8_t>&& init_tuple) : 
        FiberScaffoldType(std::make_tuple(std::get<0>(init_tuple))),
        streamver(std::get<1>(init_tuple)) {}
    
    int did_recv(auto&&, InnerMessageType&& buf);
    void did_dial(auto&&);
    ConnectionState get_connection_state() { return conn_state; }

private:
    int did_recv_DATA(InnerMessageType&& buf);
    int did_recv_DIALCONF(auto&&);
};

template<typename ExtFabric>
void ConnSMFiber<ExtFabric>::did_dial(auto&&) {
    conn_state = ConnectionState::DialSent;
}

template<typename ExtFabric>
int ConnSMFiber<ExtFabric>::did_recv_DATA(InnerMessageType&& buf) {
    if(conn_state == ConnectionState::DialRcvd) {
        conn_state = ConnectionState::Established;
        if(dialled) {
            // delegate->did_dial(*this);
        }
    } else if(conn_state != ConnectionState::Established) {
        return -1;
    }
    return this->ext_fabric->i(*this)->did_recv(*this, std::move(buf));
}

template<typename ExtFabric>
int ConnSMFiber<ExtFabric>::did_recv_DIALCONF(auto&&) {
    if(conn_state == ConnectionState::DialSent) {
        conn_state = ConnectionState::Established;
        return 0;
    }
    return -1;
}

//! Receives the packet and processes them
/*!
    Determines the type of packet by reading the first byte and redirects the packet to appropriate function for further processing

    \li \b byte :	\b type
    \li 0       :	DATA
    \li 1       :	DATA + FIN
    \li 2       :	ACK
    \li 3       :	DIAL
    \li 4       :	DIALCONF
    \li 5       :	CONF
    \li 6       :	RST
*/
template<typename ExtFabric>
int ConnSMFiber<ExtFabric>::did_recv(auto&&, InnerMessageType&& buf) {
    /* TODO: Find what to do with version? (idx 0) */
    auto type = buf.read_uint8(1);
    // SPDLOG_INFO("type value: {}", type.value());
    if (type == std::nullopt || buf.read_uint8_unsafe(0) == 0) {
        return -1;
    }

    switch (type.value())
    {
    case 0: // Data
    case 1: // Data + FIN
        return did_recv_DATA(std::move(buf));
    case 2: // Ack
        break;
    case 3: // Dial 
        break;
    case 4: // DialConf
        return did_recv_DIALCONF(std::move(buf));
    case 5: // Conf
        // did_recv_CONF(std::move(packet));
        break;
    case 6: // RST
        // did_recv_RST(std::move(packet));
        break;
    case 7: // SkipStream
        // did_recv_SKIPSTREAM(std::move(packet));
        break;
    case 8: // FlushStream
        // did_recv_FLUSHSTREAM(std::move(packet));
        break;
    case 9: // FlushConf
        // did_recv_FLUSHCONF(std::move(packet));
        break;
    default:
        break;
    }
    // Case: Default break
    return -1;
}

} // namespace stream
} // namespace marlin

#endif // MARLIN_STREAM_CONNSMFIBER_HPP