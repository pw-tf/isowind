project(tes3mp-server)

option(ENABLE_BREAKPAD "Enable Google Breakpad for Crash reporting" OFF)

if(ENABLE_BREAKPAD)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_BREAKPAD")
    if (UNIX)
        set(Breakpad_Headers "${CMAKE_SOURCE_DIR}/extern/breakpad/src/client/linux")
        set(Breakpad_Library "${CMAKE_SOURCE_DIR}/extern/breakpad/src/client/linux/libbreakpad_client.a")
    elseif(WIN32)
        set(Breakpad_Headers "${CMAKE_SOURCE_DIR}/extern/breakpad/src/client/windows")
        set(Breakpad_Library "-lbreakpad_client")
    endif (UNIX)
    include_directories(${CMAKE_SOURCE_DIR}/extern/breakpad/src ${Breakpad_Headers})
endif(ENABLE_BREAKPAD)

option(BUILD_WITH_LUA "Enable Lua language" ON)
if(BUILD_WITH_LUA)

    find_package(LuaJit REQUIRED)

    MESSAGE(STATUS "Found LuaJit_LIBRARIES: ${LuaJit_LIBRARIES}")
    MESSAGE(STATUS "Found LuaJit_INCLUDE_DIRS: ${LuaJit_INCLUDE_DIRS}")

    set(LuaScript_Sources
            Script/LangLua/LangLua.cpp
            Script/LangLua/LuaFunc.cpp
            Script/LangLua/LuaIo2.cpp)
    set(LuaScript_Headers ${LUA_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/extern/LuaBridge ${CMAKE_SOURCE_DIR}/extern/LuaBridge/detail
            Script/LangLua/LangLua.hpp
            Script/LangLua/LuaIo2.hpp)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DENABLE_LUA")
    include_directories(SYSTEM ${LuaJit_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/extern/LuaBridge)
endif(BUILD_WITH_LUA)

include_directories(${CMAKE_SOURCE_DIR}/extern/PicoSHA2)

set(NativeScript_Sources
        Script/LangNative/LangNative.cpp
        )
set(NativeScript_Headers
        Script/LangNative/LangNative.hpp
        )

# local files
set(SERVER
    main.cpp
    ServerConfig.cpp
    Player.cpp
    Networking.cpp
    MasterClient.cpp
    Cell.cpp
    CellController.cpp
    Utils.cpp
    Script/Script.cpp Script/ScriptFunction.cpp
    Script/ScriptFunctions.cpp

    Script/Functions/Actors.cpp Script/Functions/Objects.cpp Script/Functions/Miscellaneous.cpp
    Script/Functions/Worldstate.cpp

    Script/Functions/Books.cpp Script/Functions/Cells.cpp Script/Functions/CharClass.cpp
    Script/Functions/Chat.cpp Script/Functions/Dialogue.cpp Script/Functions/Factions.cpp
    Script/Functions/GUI.cpp Script/Functions/Items.cpp Script/Functions/Mechanics.cpp
    Script/Functions/Positions.cpp Script/Functions/Quests.cpp Script/Functions/RecordsDynamic.cpp
    Script/Functions/Server.cpp Script/Functions/Settings.cpp Script/Functions/Shapeshift.cpp
    Script/Functions/Spells.cpp Script/Functions/Stats.cpp Script/Functions/Timer.cpp

    Script/API/TimerAPI.cpp Script/API/PublicFnAPI.cpp
        ${LuaScript_Sources}
        ${NativeScript_Sources}

)

set(SERVER_HEADER
        Script/Types.hpp Script/Script.hpp Script/SystemInterface.hpp
        Script/ScriptFunction.hpp Script/Platform.hpp Script/Language.hpp
        Script/ScriptFunctions.hpp Script/API/TimerAPI.hpp Script/API/PublicFnAPI.hpp
        ${LuaScript_Headers}
        ${NativeScript_Headers}
)
source_group(tes3mp-server FILES ${SERVER} ${SERVER_HEADER})

set(PROCESSORS_ACTOR
        processors/actor/ProcessorActorAI.hpp processors/actor/ProcessorActorAnimFlags.hpp
        processors/actor/ProcessorActorAnimPlay.hpp processors/actor/ProcessorActorAttack.hpp
        processors/actor/ProcessorActorCast.hpp processors/actor/ProcessorActorCellChange.hpp
        processors/actor/ProcessorActorDeath.hpp processors/actor/ProcessorActorEquipment.hpp
        processors/actor/ProcessorActorList.hpp processors/actor/ProcessorActorPosition.hpp
        processors/actor/ProcessorActorSpeech.hpp processors/actor/ProcessorActorSpellsActive.hpp
        processors/actor/ProcessorActorStatsDynamic.hpp processors/actor/ProcessorActorTest.hpp
        )

source_group(tes3mp-server\\processors\\actor FILES ${PROCESSORS_ACTOR})

set(PROCESSORS_PLAYER
        processors/player/ProcessorChatMsg.hpp processors/player/ProcessorGUIMessageBox.hpp
        processors/player/ProcessorPlayerAnimFlags.hpp processors/player/ProcessorPlayerAnimPlay.hpp
        processors/player/ProcessorPlayerAttack.hpp processors/player/ProcessorPlayerAttribute.hpp
        processors/player/ProcessorPlayerBook.hpp processors/player/ProcessorPlayerBounty.hpp
        processors/player/ProcessorPlayerCast.hpp processors/player/ProcessorPlayerCellChange.hpp
        processors/player/ProcessorPlayerCellState.hpp processors/player/ProcessorPlayerCharClass.hpp
        processors/player/ProcessorPlayerCharGen.hpp processors/player/ProcessorPlayerCooldowns.hpp
        processors/player/ProcessorPlayerDeath.hpp processors/player/ProcessorPlayerDisposition.hpp
        processors/player/ProcessorPlayerEquipment.hpp processors/player/ProcessorPlayerFaction.hpp
        processors/player/ProcessorPlayerInput.hpp processors/player/ProcessorPlayerInventory.hpp
        processors/player/ProcessorPlayerItemUse.hpp processors/player/ProcessorPlayerJournal.hpp
        processors/player/ProcessorPlayerPlaceholder.hpp processors/player/ProcessorPlayerLevel.hpp
        processors/player/ProcessorPlayerMiscellaneous.hpp processors/player/ProcessorPlayerPosition.hpp
        processors/player/ProcessorPlayerQuickKeys.hpp processors/player/ProcessorPlayerRest.hpp
        processors/player/ProcessorPlayerResurrect.hpp processors/player/ProcessorPlayerShapeshift.hpp
        processors/player/ProcessorPlayerSkill.hpp processors/player/ProcessorPlayerSpeech.hpp
        processors/player/ProcessorPlayerSpellbook.hpp processors/player/ProcessorPlayerSpellsActive.hpp
        processors/player/ProcessorPlayerStatsDynamic.hpp processors/player/ProcessorPlayerTopic.hpp
        )

source_group(tes3mp-server\\processors\\player FILES ${PROCESSORS_PLAYER})

set(PROCESSORS_OBJECT
        processors/object/ProcessorConsoleCommand.hpp processors/object/ProcessorContainer.hpp
        processors/object/ProcessorDoorState.hpp processors/object/ProcessorMusicPlay.hpp
        processors/object/ProcessorObjectActivate.hpp processors/object/ProcessorObjectAnimPlay.hpp
        processors/object/ProcessorObjectDelete.hpp processors/object/ProcessorObjectDialogueChoice.hpp
        processors/object/ProcessorObjectHit.hpp processors/object/ProcessorObjectLock.hpp
        processors/object/ProcessorObjectMiscellaneous.hpp processors/object/ProcessorObjectMove.hpp
        processors/object/ProcessorObjectPlace.hpp processors/object/ProcessorObjectRestock.hpp
        processors/object/ProcessorObjectRotate.hpp processors/object/ProcessorObjectScale.hpp
        processors/object/ProcessorObjectSound.hpp processors/object/ProcessorObjectSpawn.hpp
        processors/object/ProcessorObjectState.hpp processors/object/ProcessorObjectTrap.hpp
        processors/object/ProcessorClientScriptLocal.hpp processors/object/ProcessorScriptMemberShort.hpp
        processors/object/ProcessorVideoPlay.hpp
        )

source_group(tes3mp-server\\processors\\object FILES ${PROCESSORS_OBJECT})

set(PROCESSORS_WORLDSTATE
        processors/worldstate/ProcessorClientScriptGlobal.hpp processors/worldstate/ProcessorRecordDynamic.hpp
        processors/worldstate/ProcessorWorldKillCount.hpp processors/worldstate/ProcessorWorldMap.hpp
        processors/worldstate/ProcessorWorldWeather.hpp
        )

source_group(tes3mp-server\\processors\\worldstate FILES ${PROCESSORS_WORLDSTATE})

set(PROCESSORS
        processors/ProcessorInitializer.cpp
        processors/PlayerProcessor.cpp
        processors/ActorProcessor.cpp
        processors/ObjectProcessor.cpp
        processors/WorldstateProcessor.cpp
        )

source_group(tes3mp-server\\processors FILES ${PROCESSORS})

include_directories("./")
include_directories(${CMAKE_SOURCE_DIR}/extern)

# Main executable

add_executable(tes3mp-server
        ${SERVER} ${SERVER_HEADER}
        ${PROCESSORS_ACTOR} ${PROCESSORS_PLAYER} ${PROCESSORS_OBJECT} ${PROCESSORS_WORLDSTATE} ${PROCESSORS}
        ${APPLE_BUNDLE_RESOURCES}
        )

target_compile_options(tes3mp-server PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/permissive->)
# components' precompiled header brings in sol3, whose Lua 5.3 compatibility macros clash with LuaBridge.
set_target_properties(tes3mp-server PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)

if (OPENMW_MP_BUILD)
	target_compile_options(tes3mp-server PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
endif()

# Built as C++20 like the rest of the engine: it includes OpenMW 0.51's headers.

if (UNIX)
	target_compile_options(tes3mp-server PRIVATE -Wno-ignored-qualifiers)
endif()

target_link_libraries(tes3mp-server
    #${Boost_SYSTEM_LIBRARY}
    #${Boost_THREAD_LIBRARY}
    #${Boost_FILESYSTEM_LIBRARY}
    #${Boost_PROGRAM_OPTIONS_LIBRARY}
    openmw-mp
    ${LuaJit_LIBRARIES}
    ${Breakpad_Library}
)

if (UNIX)
    target_link_libraries(tes3mp-server dl)
    # Fix for not visible pthreads functions for linker with glibc 2.15
    if(NOT APPLE)
        target_link_libraries(tes3mp-server ${CMAKE_THREAD_LIBS_INIT})
    endif(NOT APPLE)
endif(UNIX)

if (BUILD_WITH_CODE_COVERAGE)
  add_definitions (--coverage)
  target_link_libraries(tes3mp-server gcov)
endif()

if (MSVC)
    # Debug version needs increased number of sections beyond 2^16
    if (CMAKE_CL_64)
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
    endif (CMAKE_CL_64)
    add_definitions("-D_USE_MATH_DEFINES")
endif (MSVC)
