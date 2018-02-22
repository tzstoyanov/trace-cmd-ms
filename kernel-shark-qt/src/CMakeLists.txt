message("\n src ...")

message(STATUS "libkshark")
add_library(kshark SHARED libkshark.c
                          libkshark-plugin.c
                          trace-filter-hash.c)

target_link_libraries(kshark ${CMAKE_DL_LIBS}
                             ${TRACEEVENT_LIBRARY}
                             ${TRACECMD_LIBRARY} 
                             )

set_target_properties(kshark  PROPERTIES SUFFIX	".so.${KS_VERSION_STRING}")

if (PYTHONLIBS_FOUND)

    message(STATUS "kshark_wrapper")
    add_custom_target(kshark_wrapper ALL DEPENDS kshark
                      COMMENT "Generating libkshark_wrapper.c")

    add_custom_command(TARGET kshark_wrapper
                       PRE_BUILD
                       COMMAND ./pybuild.sh ${TRACECMD_LIBRARY_DIR}   ${TRACECMD_INCLUDE_DIR}
                                            ${TRACEEVENT_LIBRARY_DIR} ${TRACEEVENT_INCLUDE_DIR}
                       COMMAND cp libkshark_wrapper.so  ${KS_DIR}/bin
                       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/py)

    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
                                          "${KS_DIR}/bin/libkshark_wrapper.so"
                                          "${KS_DIR}/build/py/libkshark_wrapper.so"
                                          "${KS_DIR}/build/py/libkshark_wrapper.c")

endif (PYTHONLIBS_FOUND)

if (Qt5Widgets_FOUND AND OPENGL_FOUND AND GLUT_FOUND)

    message(STATUS "libkshark-gui")
    set (ks-guiLib_hdr  KsMainWindow.hpp
                        KsTraceViewer.hpp
                        KsTraceGraph.hpp
                        KsGLWidget.hpp
                        KsUtils.hpp)

    QT5_WRAP_CPP(ks-guiLib_hdr_moc ${ks-guiLib_hdr})

    add_library(kshark-gui  SHARED  ${ks-guiLib_hdr_moc}    KsMainWindow.cpp
                                                            KsTraceViewer.cpp
                                                            KsTraceGraph.cpp
                                                            KsModel.cpp
                                                            KsUtils.cpp
                                                            KsPlotTools.cpp
                                                            KsGLWidget.cpp)

    target_link_libraries(kshark-gui kshark
                                     ${OPENGL_LIBRARIES}
                                     ${GLUT_LIBRARY}
                                     Qt5::Widgets)

    set_target_properties(kshark-gui PROPERTIES  SUFFIX ".so.${KS_VERSION_STRING}")


    message(STATUS "kernelshark")
    add_executable(kernelshark          kernelshark.cpp)
    target_link_libraries(kernelshark   kshark-gui)

    add_subdirectory(plugins)
#     add_subdirectory(prototypes)

endif (Qt5Widgets_FOUND AND OPENGL_FOUND AND GLUT_FOUND)

configure_file( ${KS_DIR}/build/deff.h.cmake
                ${KS_DIR}/src/KsDeff.h)
