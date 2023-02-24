include(CMakeFindDependencyMacro)
find_dependency(Qt6 REQUIRED Network)

include("${CMAKE_CURRENT_LIST_DIR}/DFHackClientQtTargets.cmake")
