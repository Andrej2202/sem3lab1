cmake_minimum_required(VERSION 3.10)

# set(CMAKE_CXX_COMPILER "C:/gcc/bin/g++.exe")
# set(CMAKE_C_COMPILER "C:/gcc/bin/gcc.exe")

project(Lab1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")  <-- ЗАКОММЕНТИРОВАНО

add_executable(lab1 src/main.cpp)

# Подключаем обе директории: include и include/base
target_include_directories(lab1 PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/base
)

# Тесты
add_executable(tests
    tests/tests_main.cpp
    tests/test_ntree.cpp
    tests/test_selector.cpp
    tests/test_builder.cpp
    tests/test_interfaces.cpp
    tests/test_operators.cpp
    tests/test_sequence_range.cpp
    tests/test_edge_cases.cpp
)

target_include_directories(tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/base
    ${CMAKE_SOURCE_DIR}/tests
)

# Графическое меню (Qt6 Widgets). Цель создаётся, только если найден Qt6.
# Если Qt6 установлен не в системном префиксе, укажите его при конфигурации:
#   cmake -S . -B build -DCMAKE_PREFIX_PATH=/путь/к/qt6/usr
find_package(Qt6 COMPONENTS Widgets QUIET)
if(Qt6_FOUND)
    add_executable(lab1_gui src/main_qt.cpp)
    target_include_directories(lab1_gui PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include/base
    )
    target_link_libraries(lab1_gui PRIVATE Qt6::Widgets)
    set_target_properties(lab1_gui PROPERTIES AUTOMOC ON)
    message(STATUS "Qt6 найден (${Qt6_VERSION}) — цель lab1_gui включена")
else()
    message(STATUS "Qt6 не найден — графическая цель lab1_gui пропущена")
endif()