cmake_minimum_required(VERSION 3.1)
project(libdfhack-client-qt)

set(PUBLIC_HEADERS
	Client.h
	Function.h
	Notifier.h
	Core.h
	globals.h
)
set(SOURCES
	Client.cpp
	ClientPrivate.cpp
	Core.cpp
	Notifier.cpp
)
qt5_wrap_cpp(MOC_SOURCES
	Client.h
	Notifier.h
	ClientPrivate.h
)

protobuf_generate_cpp(PROTO_SOURCES PROTO_HEADERS
	Basic.proto
	BasicApi.proto
	CoreProtocol.proto
)
list(APPEND PUBLIC_HEADERS ${PROTO_HEADERS})

add_library(dfhack-client-qt SHARED ${SOURCES} ${MOC_SOURCES} ${PROTO_SOURCES})
qt5_use_modules(dfhack-client-qt Network)
target_include_directories(dfhack-client-qt PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>
	$<INSTALL_INTERFACE:include>
)
target_compile_definitions(dfhack-client-qt PRIVATE DFHACK_CLIENT_QT_LIBRARY)
target_link_libraries(dfhack-client-qt ${PROTOBUF_LIBRARIES})
set_target_properties(dfhack-client-qt PROPERTIES
	PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

install(TARGETS dfhack-client-qt
	EXPORT DFHackClientQtTargets
	RUNTIME DESTINATION "bin"
	LIBRARY DESTINATION "lib"
	ARCHIVE DESTINATION "lib"
	PUBLIC_HEADER DESTINATION "include/dfhack-client-qt")


