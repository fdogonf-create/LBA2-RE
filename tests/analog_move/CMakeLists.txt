# Analog movement's angle math: which way Twinsen faces for a stick held a given
# way, in a 3D exterior and in an isometric interior. Pure functions, no engine.
add_executable(test_analog_move
    test_analog_move.cpp
    ${CMAKE_SOURCE_DIR}/SOURCES/ANALOGMOVE_MATH.CPP
)
target_include_directories(test_analog_move PRIVATE
    ${CMAKE_SOURCE_DIR}/SOURCES
    ${CMAKE_SOURCE_DIR}/LIB386/H
    ${CMAKE_SOURCE_DIR}/tests
)
add_test(NAME test_analog_move COMMAND test_analog_move)
register_host_test(test_analog_move)
