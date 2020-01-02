cmake_minimum_required(VERSION 2.9 FATAL_ERROR)

add_executable(simulation
        helpers/Logger/easylogging.cpp       
        main.cpp
        core/Network/Network.cpp
        core/Network/Node/Miner.cpp
        core/Network/Node/Node.cpp
        core/Networking/RoutingTable.cpp
        core/Blockchain/Blockchain.cpp
        core/Blockchain/Block/PoSBlock.cpp
        core/Blockchain/Block/PoWBlock.cpp
        core/Blockchain/Block/Block.cpp
        core/Simulator/Simulator.cpp
        core/EventManagement/Event/Event.cpp
        core/EventManagement/Event/AsyncEvent.cpp
        core/EventManagement/EventManager/EventManager.cpp
        core/EventManagement/EventQueue/EventQueue.cpp
)

add_definitions(-DELPP_DEFAULT_LOG_FILE="./logs/default.log")

set_property(TARGET simulation PROPERTY CXX_STANDARD 11)


