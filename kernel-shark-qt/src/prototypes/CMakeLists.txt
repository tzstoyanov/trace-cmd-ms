message("\n src/prototypes ...")

add_executable(dload		dataload.c)
target_link_libraries(dload	kshark)

# add_executable(kshark-record		kshark-record.cpp)
# target_link_libraries(kshark-record	kshark-gui)

# add_executable(jsontest         jsontest.c)
# target_link_libraries(jsontest  kshark)
