cmake_minimum_required(VERSION 2.9 FATAL_ERROR)

add_executable(simulation
        main.cpp
        core/Blockchain/Blockchain.cpp
        core/Blockchain/Block/PoSBlock.cpp
        core/Blockchain/Block/PoWBlock.cpp
        core/Blockchain/Block/Block.cpp
        core/Blockchain/Cache/BlockCache.cpp
        core/Simulator/Simulator.cpp
        core/EventManagement/Event/Event.cpp
        core/EventManagement/Event/AsyncEvent.cpp
        core/EventManagement/Event/EventTypes/MessageToNodeEvent.cpp
        core/EventManagement/EventManager/EventManager.cpp
        core/EventManagement/EventQueue/EventQueue.cpp
        core/Network/Network.cpp
        core/Network/Messages/Message.cpp
        core/Network/Messages/NewBlockIdMessage.cpp
        core/Network/Node/Miner.cpp
        core/Network/Node/Node.cpp
        core/Networking/RoutingTable.cpp
        helpers/Logger/easylogging.cpp       
        models/BlockchainManagementModels/BlockchainManagementBitcoinModel.cpp
)

add_definitions(-DELPP_DEFAULT_LOG_FILE="./logs/default.log")

set_property(TARGET simulation PROPERTY CXX_STANDARD 11)


