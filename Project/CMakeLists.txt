set(CUSTOM_CMAKE_MODULE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
list(APPEND CMAKE_MODULE_PATH "${CUSTOM_CMAKE_MODULE_DIR}")


cmake_minimum_required(VERSION 3.15)


#TODO: change it to your real project name.
# Yuna: a NPR & PBR charactor
project(Kaleidoscope VERSION 0.1.0 DESCRIPTION "Light rendering engine based on Vulkan." LANGUAGES CXX)


set(PROJECT_BASE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(PROJECT_SOURCE_CODE_ROOT "${PROJECT_BASE_ROOT}/Source")
set(PROJECT_BINARY_ROOT "${PROJECT_BASE_ROOT}/Binary")
set(PROJECT_BUILD_ROOT "${PROJECT_BASE_ROOT}/Build")


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMakeTargets")

#set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_ROOT}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_ROOT}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_ROOT}")
#set(LIBRARY_OUTPUT_PATH "${PROJECT_BUILD_ROOT}/Lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BUILD_ROOT}/Lib/Debug")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BUILD_ROOT}/Lib/Release")
#set(CMAKE_PDB_OUTPUT_DIRECTORY "${PROJECT_BINARY_ROOT}");
set(CMAKE_DEBUG_POSTFIX "_D")


#if (WIN32)
#add_definitions(-DUNICODE -D_UNICODE)
#endif()


#add_subdirectory("Third-Party")
add_subdirectory("Runtime")
#add_subdirectory("Framework")
add_subdirectory("Application")


add_dependencies(GraphicDriver CrossPlatform)
add_dependencies(Player GraphicDriver)
