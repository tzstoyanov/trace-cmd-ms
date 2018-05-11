message("\n src/plugins ...")

function(ADD_PLUGIN)
    set(options )
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCE)
    cmake_parse_arguments(ADD_PLUGIN "${options}"
                                     "${oneValueArgs}"
                                     "${multiValueArgs}"
                                     ${ARGN} )

    message(STATUS ${ADD_PLUGIN_NAME})
    add_library(${ADD_PLUGIN_NAME} SHARED ${ADD_PLUGIN_SOURCE})
    set_target_properties(${ADD_PLUGIN_NAME} PROPERTIES PREFIX "plugin-")
    target_link_libraries(${ADD_PLUGIN_NAME} kshark-gui)

endfunction()

set(PLUGIN_LIST "")

ADD_PLUGIN(NAME sched_events
           SOURCE sched_events.c SchedEvents.cpp)

# list(APPEND PLUGIN_LIST "sched_events default") # This plugin will be loaded by default
list(APPEND PLUGIN_LIST "sched_events") # This plugin will be loaded by default

ADD_PLUGIN(NAME xenomai_sched_events
           SOURCE xenomai_sched_events.c XenomaiSchedEvents.cpp)

list(APPEND PLUGIN_LIST "xenomai_sched_events") # This plugin isn't loaded by default

set(PLUGINS ${PLUGIN_LIST} PARENT_SCOPE)
