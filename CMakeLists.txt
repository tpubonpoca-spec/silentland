cmake_minimum_required(VERSION 3.18)
project(dppbotcpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(DPPBOT_SHARED_SOURCES
    src/vpk_archive.cpp
    src/vpk_writer.cpp
    src/mod_analyzer.cpp
    src/site_metadata.cpp
    src/pack_library.cpp
)

add_executable(dppbotcpp WIN32
    src/gui_main.cpp
    ${DPPBOT_SHARED_SOURCES}
)

add_executable(dppbotcpp_cli
    src/main.cpp
    src/cli.cpp
    ${DPPBOT_SHARED_SOURCES}
)

target_include_directories(dppbotcpp PRIVATE include)
target_include_directories(dppbotcpp_cli PRIVATE include)
target_compile_definitions(dppbotcpp PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_definitions(dppbotcpp_cli PRIVATE _CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(dppbotcpp PRIVATE winhttp comdlg32 shell32 ole32)
target_link_libraries(dppbotcpp_cli PRIVATE winhttp)

if (MINGW)
    target_link_options(dppbotcpp PRIVATE -static -static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic)
    target_link_options(dppbotcpp_cli PRIVATE -static -static-libgcc -static-libstdc++ -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic)
endif()
