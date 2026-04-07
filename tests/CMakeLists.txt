include(FetchContent)

# Скачиваем Catch2 v3
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.13.0
)
FetchContent_MakeAvailable(Catch2)

# Укажите реальные имена ваших тестовых файлов (лежат в папке tests)
set(TEST_SOURCES
    test_evaluator.cpp
    test_derivative.cpp
    test_evaluate_derivative.cpp
)

add_executable(unit_tests ${TEST_SOURCES})

# Линкуем с нашей библиотекой calc_lib (не graph_lib)
target_link_libraries(unit_tests PRIVATE
    calc_lib
    Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(unit_tests)
