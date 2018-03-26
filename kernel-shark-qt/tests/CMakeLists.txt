message("\n tests ...")

if (Qt5Widgets_FOUND AND OPENGL_FOUND AND GLUT_FOUND)

    message(STATUS "tests")
    add_executable(tests		tests.cpp)
    target_link_libraries(tests	gtest	kshark-gui)

    add_executable(migrate		migrate_new.c)

endif (Qt5Widgets_FOUND AND OPENGL_FOUND AND GLUT_FOUND)
