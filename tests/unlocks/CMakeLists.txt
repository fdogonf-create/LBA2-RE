add_executable(test_unlocks
    test_unlocks.cpp
    ${CMAKE_SOURCE_DIR}/SOURCES/UNLOCKS.CPP
)

target_include_directories(test_unlocks PRIVATE
    ${CMAKE_SOURCE_DIR}/SOURCES
)

set_target_properties(test_unlocks PROPERTIES CXX_STANDARD 11)

add_test(NAME test_unlocks COMMAND test_unlocks)

register_host_test(test_unlocks)
