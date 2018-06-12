message("\n src ...")

message(STATUS "libkshark")
add_library(kshark SHARED libkshark.c
                          libkshark-model.c
                          libkshark-plugin.c
                          libkshark-collection.c
                          trace-filter-hash.c
                          libkshark-json.c
                          )

target_link_libraries(kshark ${CMAKE_DL_LIBS}
                             ${TRACEEVENT_LIBRARY}
                             ${TRACECMD_LIBRARY} 
                             ${JSONC_LIBRARY})

set_target_properties(kshark  PROPERTIES SUFFIX	".so.${KS_VERSION_STRING}")

if (PYTHONLIBS_FOUND AND CYTHON_FOUND AND NUMPY_FOUND)

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

endif (PYTHONLIBS_FOUND AND CYTHON_FOUND AND NUMPY_FOUND)

if (Qt5Widgets_FOUND AND Qt5Network_FOUND AND OPENGL_FOUND AND GLUT_FOUND)

    message(STATUS "libkshark-gui")
    set (ks-guiLib_hdr  KsMainWindow.hpp
                        KsWidgetsLib.hpp
                        KsTraceViewer.hpp
                        KsTraceGraph.hpp
                        KsModel.hpp
                        KsGLWidget.hpp
                        KsUtils.hpp
                        KsCaptureDialog.hpp
                        KsAdvFilteringDialog.hpp)

    QT5_WRAP_CPP(ks-guiLib_hdr_moc ${ks-guiLib_hdr})

    add_library(kshark-gui  SHARED  ${ks-guiLib_hdr_moc}    KsMainWindow.cpp
                                                            KsWidgetsLib.cpp
                                                            KsTraceViewer.cpp
                                                            KsTraceGraph.cpp
                                                            KsModel.cpp
                                                            KsUtils.cpp
                                                            KsPlotTools.cpp
                                                            KsGLWidget.cpp
                                                            KsCaptureDialog.cpp
                                                            KsAdvFilteringDialog.cpp)

    target_link_libraries(kshark-gui kshark
                                     ${OPENGL_LIBRARIES}
                                     ${GLUT_LIBRARY}
                                     Qt5::Widgets
                                     Qt5::Network
                                     Qt5::Concurrent)

    set_target_properties(kshark-gui PROPERTIES  SUFFIX ".so.${KS_VERSION_STRING}")


    message(STATUS "kernelshark")
    add_executable(kernelshark          kernelshark.cpp)
    target_link_libraries(kernelshark   kshark-gui)

    message(STATUS "kshark-record")
    add_executable(kshark-record        kshark-record.cpp)
    target_link_libraries(kshark-record kshark-gui)

    add_subdirectory(plugins)
    add_subdirectory(prototypes)

endif (Qt5Widgets_FOUND AND Qt5Network_FOUND AND OPENGL_FOUND AND GLUT_FOUND)

find_program(LSB_RELEASE lsb_release)
execute_process(COMMAND ${LSB_RELEASE} -is
    OUTPUT_VARIABLE LSB_DISTRIB
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# find_program(DO_AS_ROOT pkexec)
find_program(DO_AS_ROOT gksu)
if (NOT DO_AS_ROOT)

    find_program(DO_AS_ROOT beesu)

endif (NOT DO_AS_ROOT)

set(DESKTOP_SESSION $ENV{DESKTOP_SESSION})

configure_file( ${KS_DIR}/build/deff.h.cmake
                ${KS_DIR}/src/KsDeff.h)
