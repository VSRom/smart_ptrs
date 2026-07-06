cmake_minimum_required(VERSION 3.14)
project(MySmartPointers CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(run_tests tests.cpp srt-ptr.h)

find_package(Threads REQUIRED)
target_link_libraries(run_tests PRIVATE Threads::Threads)

enable_testing()

# Добавляем тест с явным указанием working directory
add_test(NAME SmartPtrTests 
    COMMAND $<TARGET_FILE:run_tests>
    WORKING_DIRECTORY $<TARGET_FILE_DIR:run_tests>
)