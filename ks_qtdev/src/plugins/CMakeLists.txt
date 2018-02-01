message("\n src/plugins ...")

function(add_plugin PLUGIN_NAME PLUGIN_SOURCE)

    message(STATUS "${PLUGIN_NAME}")
    add_library(${PLUGIN_NAME} SHARED ${PLUGIN_SOURCE} SchedEvents.cpp)
    set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "plugin-")
    target_link_libraries(${PLUGIN_NAME} kshark-gui)

endfunction()

set(PLUGIN_LIST "")

set(P_NAME   "sched_events")
set(P_SOURCE "sched_events.c; SchedEvents.cpp")
add_plugin(${P_NAME} ${P_SOURCE})
list(APPEND PLUGIN_LIST ${P_NAME})

set(P_NAME   "xenomai_sched_events")
set(P_SOURCE "xenomai_sched_events.c; XenomaiSchedEvents.cpp")
add_plugin(${P_NAME} ${P_SOURCE})
list(APPEND PLUGIN_LIST ${P_NAME})

set(PLUGINS ${PLUGIN_LIST} PARENT_SCOPE)

