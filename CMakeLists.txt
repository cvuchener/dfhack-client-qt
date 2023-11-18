cmake_minimum_required(VERSION 3.5)
project(dfhack-remote-client-qt)

option(BUILD_TEST "build test" OFF)
option(BUILD_CONSOLE_EXAMPLE "build DFHack remote console example" OFF)

find_package(Qt6 REQUIRED Network OPTIONAL_COMPONENTS Widgets)
find_package(Protobuf REQUIRED)

add_subdirectory(dfhack-client-qt)
if (BUILD_TEST)
	add_subdirectory(test)
endif()
if (BUILD_CONSOLE_EXAMPLE)
	if (Qt6Widgets_FOUND)
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
