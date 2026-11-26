add_library(pl_mpeg STATIC pl_mpeg.c)
target_include_directories(pl_mpeg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(pl_mpeg PROPERTIES C_STANDARD 99)
