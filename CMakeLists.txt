cmake_minimum_required(VERSION 3.17)

project(vga_debug_generator)

include(cmake/CPM.cmake)

CPMAddPackage(
    NAME nlohmann_json
    VERSION 3.9.1
    URL https://github.com/nlohmann/json/releases/download/v3.9.1/include.zip
    
)
if (nlohmann_json_ADDED)
    add_library(nlohmann_json INTERFACE IMPORTED)
    target_include_directories(nlohmann_json INTERFACE ${nlohmann_json_SOURCE_DIR}/include)
endif()

add_subdirectory(src)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE VgaDebugGenerator)