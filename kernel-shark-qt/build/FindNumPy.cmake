execute_process(COMMAND python -c "import Cython; print(Cython.__version__)"
                RESULT_VARIABLE CYTHON_RES
                OUTPUT_VARIABLE CYTHON_VERSION
                ERROR_VARIABLE CYTHON_ERR
                OUTPUT_STRIP_TRAILING_WHITESPACE)

IF (CYTHON_RES MATCHES 0)

  SET(CYTHON_FOUND TRUE)
  message(STATUS "Found Cython:  (version: ${CYTHON_VERSION})")

ELSE (CYTHON_RES MATCHES 0)

  SET(CYTHON_FOUND FALSE)
  message(STATUS "\nCould not find CYTHON:  ${CYTHON_ERR}\n")

ENDIF (CYTHON_RES MATCHES 0)

execute_process(COMMAND python -c "import numpy; print(numpy.__version__)"
                RESULT_VARIABLE NP_RES
                OUTPUT_VARIABLE NUMPY_VERSION
                ERROR_VARIABLE NP_ERR
                OUTPUT_STRIP_TRAILING_WHITESPACE)

IF (NP_RES MATCHES 0)

  SET(NUMPY_FOUND TRUE)
  message(STATUS "Found NumPy:  (version: ${NUMPY_VERSION})")

ELSE (NP_RES MATCHES 0)

  SET(NUMPY_FOUND FALSE)
  message(STATUS "\nCould not find NumPy:  ${NP_ERR}\n")

ENDIF (NP_RES MATCHES 0)
