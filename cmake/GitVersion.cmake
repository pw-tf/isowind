execute_process (
    COMMAND ${GIT_EXECUTABLE} rev-list --tags --max-count=1
    WORKING_DIRECTORY ${OpenMW_SOURCE_DIR}
    RESULT_VARIABLE EXITCODE1
    OUTPUT_VARIABLE TAGHASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)

# Isowind: the hash of the source tree rather than of the commit. Multiplayer only lets the same version play
# together (the hash is part of the connection password), and the tree is the same wherever that source is built:
# here, and from the snapshot commits pushed to GitHub for the Linux and Android builds.
execute_process (
    COMMAND ${GIT_EXECUTABLE} rev-parse HEAD^{tree}
    WORKING_DIRECTORY ${OpenMW_SOURCE_DIR}
    RESULT_VARIABLE EXITCODE2
    OUTPUT_VARIABLE COMMITHASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)

string (COMPARE EQUAL "${EXITCODE1}:${EXITCODE2}" "0:0" FULL_SUCCESS)
string (COMPARE EQUAL "${EXITCODE2}" "0" COMMIT_SUCCESS)
if (FULL_SUCCESS)
    set(OPENMW_VERSION_COMMITHASH "${COMMITHASH}")
    set(OPENMW_VERSION_TAGHASH "${TAGHASH}")
    message(STATUS "OpenMW version ${OPENMW_VERSION}")
elseif (COMMIT_SUCCESS)
    set(OPENMW_VERSION_COMMITHASH "${COMMITHASH}")
    message(STATUS "OpenMW version ${OPENMW_VERSION}")
else ()
    message(WARNING "Failed to get valid version information from Git")
endif ()

include(${MACROSFILE})

configure_resource_file(${VERSION_RESOURCE_FILE_IN} ${OpenMW_BINARY_DIR} ${VERSION_RESOURCE_FILE_RELATIVE})
configure_file("${VERSION_CPP_FILE_IN}" "${VERSION_CPP_FILE_OUT}")
