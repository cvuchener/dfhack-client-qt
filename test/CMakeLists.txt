cmake_minimum_required(VERSION 3.5)
project(dfhack-qt-console)

add_executable(test-chain test-chain.cpp)
target_link_libraries(test-chain DFHackClientQt::dfhack-client-qt)
add_executable(test-sync test-sync.cpp)
target_link_libraries(test-sync DFHackClientQt::dfhack-client-qt)
