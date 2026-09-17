include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(b_plus_tree_test
    tests/index/b_plus_tree_test.cpp
    src/index/b_plus_tree/b_plus_tree.cpp
    src/index/b_plus_tree/internal_page.cpp
    src/index/b_plus_tree/leaf_page.cpp
    src/storage/buffer_manager.cpp
)
target_link_libraries(b_plus_tree_test gtest gtest_main pthread)
