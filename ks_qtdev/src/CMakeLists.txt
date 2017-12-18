MESSAGE(" src ...")

add_library(kshark SHARED libkshark.c)
target_link_libraries(kshark ${CMAKE_DL_LIBS} ${TRACECMD_LIBRARY})
set_target_properties(kshark  PROPERTIES SUFFIX	".so.${KS_VERSION_STRING}")

IF (PYTHONLIBS_FOUND)

    add_custom_target(kshark_wrapper ALL DEPENDS kshark
                      COMMENT "Generating libkshark_wrapper.c")

    set(NPY_BUILD python np_setup.py --library=tracecmd
                                     --libdir=${TRACECMD_LIBRARY_DIR}
                                     build_ext -i
                                     > pybuild.log)

    add_custom_command(TARGET kshark_wrapper
                       PRE_BUILD
                       COMMAND ./pybuild.sh ${TRACECMD_LIBRARY_DIR}
                       COMMAND cp libkshark_wrapper.so  ${KS_DIR}/bin
                       WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/py)

    set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
                                          "${KS_DIR}/bin/libkshark_wrapper.so"
                                          "${KS_DIR}/build/py/libkshark_wrapper.so"
                                          "${KS_DIR}/build/py/libkshark_wrapper.c")

ENDIF (PYTHONLIBS_FOUND)

IF (Qt5Widgets_FOUND)

    set (ks-guiLib_hdr	KsMainWindow.h
			KsTraceViewer.h
			KsTraceGraph.h
			KsUtils.h)

    QT5_WRAP_CPP(ks-guiLib_hdr_moc ${ks-guiLib_hdr})

    add_library(kshark-gui  SHARED  ${ks-guiLib_hdr_moc}	KsMainWindow.cpp
								KsTraceViewer.cpp
								KsTraceGraph.cpp
								KsModel.cpp
								KsUtils.cpp
								KsPlotTools.cpp)

    target_link_libraries(kshark-gui ${TRACECMD_LIBRARY}
				     kshark
				     GLU GL glut
				     Qt5::Widgets)

    set_target_properties(kshark-gui  PROPERTIES  SUFFIX	".so.${KS_VERSION_STRING}")


    MESSAGE(STATUS "kernelshark")
    add_executable(kernelshark		kernelshark.cpp)
    target_link_libraries(kernelshark	kshark-gui)

    add_executable(draw		draw.cpp)
    target_link_libraries(draw	kshark-gui)

ENDIF (Qt5Widgets_FOUND)

add_executable(dload		dataload.c)
target_link_libraries(dload	kshark)
