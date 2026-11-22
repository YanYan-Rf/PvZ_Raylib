cmake_minimum_required(VERSION 3.20)
project(PvZ_Raylib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 通过 vcpkg 查找 raylib 与头文件库 raygui
find_package(raylib CONFIG REQUIRED)
find_path(RAYGUI_INCLUDE_DIRS "raygui.h" REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/Game.cpp
    src/Game.hpp
    src/Types.hpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${RAYGUI_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

if(MSVC)
    # 避免 raygui/raylib 中 C 标准库函数警告与 Windows min/max 宏冲突
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
    )
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /W3)
endif()