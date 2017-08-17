# Find trace-cmd
# This module finds an installed trace-cmd package.
#
# It sets the following variables:
#  TRACECMD_INCLUDE_DIR, where to find header, etc.
#  TRACECMD_LIBRARY, the libraries needed to use trace-cmd.
#  TRACECMD_FOUND, If false, do not try to use trace-cmd.

# MESSAGE(" Looking for trace-cmd ...")

find_path(	TRACECMD_INCLUDE_DIR	 NAMES	 event-parse.h
			PATHS	$ENV{TRACE_CMD}	${CMAKE_SOURCE_DIR}/../)

find_library(	TRACECMD_LIBRARY	NAMES	libtracecmd.a
				PATHS	$ENV{TRACE_CMD}	${CMAKE_SOURCE_DIR}/../)

find_library(	TRACECMD_LIBRARY_PE	NAMES	parsevent
				PATHS	$ENV{TRACE_CMD}	${CMAKE_SOURCE_DIR}/../)

IF (TRACECMD_INCLUDE_DIR AND TRACECMD_LIBRARY AND TRACECMD_LIBRARY_PE)

  SET(TRACECMD_FOUND TRUE)
#   SET(TRACECMD_LIBRARY ${TRACECMD_LIBRARY} ${TRACECMD_LIBRARY_PE})

ENDIF (TRACECMD_INCLUDE_DIR AND TRACECMD_LIBRARY AND TRACECMD_LIBRARY_PE)

IF (TRACECMD_FOUND)

  MESSAGE(STATUS "Found trace-cmd: ${TRACECMD_LIBRARY}\n")

ELSE (TRACECMD_FOUND)

  MESSAGE(FATAL_ERROR "Could not find trace-cmd!\n")

ENDIF (TRACECMD_FOUND)
