MESSAGE(" src ...")

add_library(trace-store SHARED trace-view-store.c ks-view.c)
target_link_libraries(trace-store ${TRACECMD_LIBRARY})
set_target_properties(trace-store	PROPERTIES SUFFIX	".so.${KS_VERSION_STRING}")

set (ks-guiLib_hdr	KsMainWindow.h
					KsTraceViewer.h
					KsTraceGraph.h
					KsCheckBoxDialog.h)

qt5_wrap_cpp(ks-guiLib_hdr_moc ${ks-guiLib_hdr})
# qt5_wrap_ui (ks-guiLib_ui_moc  ${ks-guiLib_ui})

add_library(kshark	SHARED	${ks-guiLib_hdr_moc}
							KsMainWindow.cpp
							KsTraceViewer.cpp
							KsTraceGraph.cpp
							KsModel.cpp
							KsDataStore.cpp
							KsCheckBoxDialog.cpp)

target_link_libraries(kshark	Qt5::Widgets
										Qt5::Charts
										trace-store)

set_target_properties(kshark	PROPERTIES SUFFIX	".so.${KS_VERSION_STRING}")


MESSAGE(STATUS "kernelshark")
add_executable (kernelshark			kernelshark.cpp)
target_link_libraries(kernelshark	kshark)

# add_executable(dload			dataload.c)
# target_link_libraries(dload		trace-store)

add_executable(dload			dataload.cpp)
target_link_libraries(dload		kshark)

MESSAGE("")

