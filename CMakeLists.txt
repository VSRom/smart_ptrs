cmake_minimum_required(VERSION 3.14)
project(MySmartPointers CXX)

# Включаем стандарт C++17 или выше (необходим для std::exchange и атомиков)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем исполняемый файл для тестов
add_executable(run_tests tests.cpp)

# Для Linux систем добавляем поддержку потоков (pthread)
find_package(Threads REQUIRED)
target_link_libraries(run_tests PRIVATE Threads::Threads)