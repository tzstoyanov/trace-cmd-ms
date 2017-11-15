MESSAGE(" src ...")

add_library(kshark SHARED libkshark.c)
target_link_libraries(kshark ${CMAKE_DL_LIBS} ${TRACECMD_LIBRARY})
set_target_properties(kshark  PROPERTIES SUFFIX	".so.${KS_VERSION_STRING}")

IF (Qt5Widgets_FOUND AND Qt5Charts_FOUND)
    set (ks-guiLib_hdr	KsMainWindow.h
			KsTraceViewer.h
			KsTraceGraph.h
			KsUtils.h)

    qt5_wrap_cpp(ks-guiLib_hdr_moc ${ks-guiLib_hdr})

    add_library(kshark-gui  SHARED  ${ks-guiLib_hdr_moc}	KsMainWindow.cpp
								KsTraceViewer.cpp
								KsTraceGraph.cpp
								KsModel.cpp
								KsUtils.cpp)

    target_link_libraries(kshark-gui ${TRACECMD_LIBRARY}	kshark
								Qt5::Widgets
								Qt5::Charts)

    set_target_properties(kshark-gui  PROPERTIES  SUFFIX	".so.${KS_VERSION_STRING}")


	MESSAGE(STATUS "kernelshark")
	add_executable(kernelshark		kernelshark.cpp)
	target_link_libraries(kernelshark	kshark-gui)

ENDIF (Qt5Widgets_FOUND AND Qt5Charts_FOUND)

add_executable(dload			dataload.c)
target_link_libraries(dload		kshark)

MESSAGE("")

