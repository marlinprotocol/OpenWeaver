cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(marlin VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(vendor)

add_subdirectory(net)
add_subdirectory(beacon)
add_subdirectory(stream)
add_subdirectory(lpf)
add_subdirectory(pubsub)
add_subdirectory(multicastsdk)
add_subdirectory(relay)
add_subdirectory(goldfish)
add_subdirectory(simulator)
add_subdirectory(des)
