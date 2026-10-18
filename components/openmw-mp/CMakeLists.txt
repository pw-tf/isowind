# Isowind multiplayer: TES3MP's networking (packets, shared player / actor / object / world state structures and
# their controllers), used by both the client and the server. Its own library rather than part of `components`, so
# single-player builds don't depend on RakNet.

file(GLOB_RECURSE OPENMW_MP_FILES CONFIGURE_DEPENDS *.cpp *.hpp)

add_library(openmw-mp STATIC ${OPENMW_MP_FILES})

target_include_directories(openmw-mp PUBLIC ${RakNet_INCLUDES} ${OpenMW_SOURCE_DIR}/extern/PicoSHA2)
target_link_libraries(openmw-mp PUBLIC components ${RakNet_LIBRARY})

if (WIN32)
    # RakNet uses Winsock.
    target_link_libraries(openmw-mp PUBLIC ws2_32)
endif()
if (MSVC)
    target_compile_options(openmw-mp PRIVATE /permissive- /bigobj)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${OPENMW_MP_FILES})
