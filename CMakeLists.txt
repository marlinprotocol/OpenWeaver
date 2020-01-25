cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(marlin VERSION 0.0.1 LANGUAGES CXX)

add_subdirectory(vendor EXCLUDE_FROM_ALL)

add_subdirectory(net EXCLUDE_FROM_ALL)
add_subdirectory(beacon EXCLUDE_FROM_ALL)
add_subdirectory(stream EXCLUDE_FROM_ALL)
add_subdirectory(lpf EXCLUDE_FROM_ALL)
