add_library(VgaDebugGenerator
    VgaDebugGenerator.cpp
    Config.cpp
    Template.cpp
)

target_compile_features(VgaDebugGenerator PUBLIC cxx_std_17)

target_include_directories(VgaDebugGenerator
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(VgaDebugGenerator
    PRIVATE nlohmann_json::nlohmann_json
)