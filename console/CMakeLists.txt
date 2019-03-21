cmake_minimum_required(VERSION 3.1)
project(dfhack-qt-console)

set(SOURCES
	MainWindow.cpp
	main.cpp
)
qt5_wrap_cpp(MOC_SOURCES
	MainWindow.h
)
qt5_wrap_ui(UI_SOURCES
	MainWindow.ui
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_BINARY_DIR}/..
	${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(dfhack-qt-console ${SOURCES} ${UI_SOURCES} ${MOC_SOURCES})
qt5_use_modules(dfhack-qt-console Widgets Network)
target_link_libraries(dfhack-qt-console dfhack-client-qt ${PROTOBUF_LIBRARIES})

install(TARGETS dfhack-qt-console
	RUNTIME DESTINATION bin)
