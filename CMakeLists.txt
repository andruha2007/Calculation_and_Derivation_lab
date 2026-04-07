cmake_minimum_required(VERSION 3.15)
project(CalculatorLab VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(include external)

file(GLOB_RECURSE LIB_SOURCES
    "src/lexer.cpp"
    "src/parser.cpp"
    "src/visitor.cpp"
)
list(FILTER LIB_SOURCES EXCLUDE REGEX "src/main.cpp")

add_library(calc_lib STATIC ${LIB_SOURCES})
target_include_directories(calc_lib PUBLIC include)

enable_testing()

# Список тестов — test_main.cpp должен быть первым
set(TEST_SOURCES
    tests/test_main.cpp
    tests/test_evaluator.cpp
    # добавьте остальные ваши тестовые файлы
)

add_executable(unit_tests ${TEST_SOURCES})
target_link_libraries(unit_tests PRIVATE calc_lib)

add_test(NAME unit_tests COMMAND unit_tests)