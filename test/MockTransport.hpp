#ifndef MOCK_TRANSPORT_HPP
#define MOCK_TRANSPORT_HPP

#include <marlin/net/Packet.hpp>
#include <marlin/net/SocketAddress.hpp>

template<template<typename> class ProtocolType, typename StorageType>
struct MockTransport {
	std::function<void(marlin::net::Packet &&, const marlin::net::SocketAddress &)> send;
	StorageType protocol_storage;
};

#endif // MOCK_TRANSPORT_HPP
