include(CMakeFindDependencyMacro)
find_dependency(Qt5 REQUIRED Network)

include("${CMAKE_CURRENT_LIST_DIR}/DFHackClientQtTargets.cmake")
