MESSAGE(" tests ...")

IF (Qt5Widgets_FOUND AND OPENGL_FOUND AND GLUT_FOUND)

    add_executable (tests		tests.cpp)
    target_link_libraries(tests	gtest	kshark-gui)

ENDIF (Qt5Widgets_FOUND AND OPENGL_FOUND AND GLUT_FOUND)
