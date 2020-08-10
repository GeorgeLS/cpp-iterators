cmake_minimum_required(VERSION 3.17)
project(iterator)

set(CMAKE_CXX_STANDARD 17)

add_executable(array_test iterator.h data_structures/array.h unit_test.h tests/array_test.cpp)
add_executable(iterator_test iterator.h data_structures/array.h unit_test.h tests/iterator_test.cpp)
add_executable(range_test iterator.h data_structures/range.h unit_test.h tests/range_test.cpp)