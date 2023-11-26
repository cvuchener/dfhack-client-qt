cmake_minimum_required(VERSION 3.5)
project(libdfhack-client-qt)

set(PUBLIC_HEADERS
	Client.h
	CommandResult.h
	Function.h
	Core.h
	Basic.h
	globals.h
)
set(SOURCES
	Client.cpp
	CommandResult.cpp
)
qt6_wrap_cpp(MOC_SOURCES
	Client.h
)

protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS
	EXPORT_MACRO DFHACK_CLIENT_QT_EXPORT
	Basic.proto
	BasicApi.proto
	CoreProtocol.proto
)
list(APPEND PUBLIC_HEADERS ${PROTO_HEADERS})

add_library(dfhack-client-qt ${SOURCES} ${MOC_SOURCES} ${PROTO_SOURCES})
target_compile_features(dfhack-client-qt PUBLIC cxx_std_20)
target_include_directories(dfhack-client-qt PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	$<INSTALL_INTERFACE:include>
)
target_compile_definitions(dfhack-client-qt PRIVATE DFHACK_CLIENT_QT_LIBRARY)
if (MSVC)
	target_compile_options(dfhack-client-qt PRIVATE "-FI dfhack-client-qt/globals.h")
else()
	target_compile_options(dfhack-client-qt PRIVATE -include dfhack-client-qt/globals.h)
endif()
target_link_libraries(dfhack-client-qt protobuf::libprotobuf Qt::Network)
set_target_properties(dfhack-client-qt PROPERTIES
	PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

install(TARGETS dfhack-client-qt
	EXPORT DFHackClientQtTargets
	RUNTIME DESTINATION "bin"
	LIBRARY DESTINATION "lib"
	ARCHIVE DESTINATION "lib"
	PUBLIC_HEADER DESTINATION "include/dfhack-client-qt")


