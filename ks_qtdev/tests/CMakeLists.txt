MESSAGE(" tests ...")

add_executable (tests			tests.cpp)
target_link_libraries(tests		gtest kshark)
