cmake_minimum_required(VERSION 3.5)
project(dfhack-qt-console)

add_executable(test-chain test-chain.cpp)
target_link_libraries(test-chain dfhack-client-qt ${PROTOBUF_LIBRARIES})
add_executable(test-sync test-sync.cpp)
target_link_libraries(test-sync dfhack-client-qt ${PROTOBUF_LIBRARIES})
