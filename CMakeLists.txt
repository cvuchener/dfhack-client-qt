cmake_minimum_required(VERSION 3.1)
project(dfhack-remote-client-qt)

option(BUILD_CONSOLE_EXAMPLE "build DFHack remote console example" OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 REQUIRED Network OPTIONAL_COMPONENTS Widgets)
find_package(Protobuf REQUIRED)

add_subdirectory(dfhack-client-qt)
if (BUILD_CONSOLE_EXAMPLE)
	if (Qt5Widgets_FOUND)
		add_subdirectory(console)
	else()
		message(FATAL_ERROR "Building console example requires Qt Widgets")
	endif()
endif()

install(EXPORT DFHackClientQtTargets
	FILE DFHackClientQtTargets.cmake
	NAMESPACE DFHackClientQt::
	DESTINATION lib/cmake/DFHackClientQt)
install(FILES cmake/DFHackClientQtConfig.cmake
	DESTINATION lib/cmake/DFHackClientQt)
