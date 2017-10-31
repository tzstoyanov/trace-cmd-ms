MESSAGE(" src ...")

add_library(trace-store SHARED trace-view-store.c ks-view.c)
target_link_libraries(trace-store ${CMAKE_DL_LIBS} ${TRACECMD_LIBRARY})
set_target_properties(trace-store  PROPERTIES SUFFIX	".so.${KS_VERSION_STRING}")

set (ks-guiLib_hdr	KsMainWindow.h
			KsTraceViewer.h
			KsTraceGraph.h
			KsUtils.h)

qt5_wrap_cpp(ks-guiLib_hdr_moc ${ks-guiLib_hdr})

add_library(kshark  SHARED  ${ks-guiLib_hdr_moc}	KsMainWindow.cpp
							KsTraceViewer.cpp
							KsTraceGraph.cpp
							KsModel.cpp
							KsUtils.cpp)

target_link_libraries(kshark  ${TRACECMD_LIBRARY}	trace-store
							Qt5::Widgets
							Qt5::Charts)

set_target_properties(kshark  PROPERTIES  SUFFIX	".so.${KS_VERSION_STRING}")


MESSAGE(STATUS "kernelshark")
add_executable (kernelshark		kernelshark.cpp)
target_link_libraries(kernelshark	kshark)

add_executable(draw			draw.cpp)
target_link_libraries(draw		kshark)

add_executable(dload			dataload.c)
target_link_libraries(dload		kshark)

MESSAGE("")

