message("\n src/prototypes ...")

add_executable(dload		dataload.c)
target_link_libraries(dload	kshark)
