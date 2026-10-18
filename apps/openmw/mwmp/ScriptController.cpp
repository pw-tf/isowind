#include <components/openmw-mp/Base/BaseStructs.hpp>

#include "../mwscript/interpretercontext.hpp"

#include "ScriptController.hpp"

unsigned short ScriptController::getPacketOriginFromContextType(unsigned short contextType)
{
    if (contextType == Interpreter::Context::CONSOLE)
        return mwmp::CLIENT_CONSOLE;
    else if (contextType == Interpreter::Context::DIALOGUE)
        return mwmp::CLIENT_DIALOGUE;
    else if (contextType == Interpreter::Context::SCRIPT_LOCAL)
        return mwmp::CLIENT_SCRIPT_LOCAL;
    else if (contextType == Interpreter::Context::SCRIPT_GLOBAL)
        return mwmp::CLIENT_SCRIPT_GLOBAL;

    return mwmp::CLIENT_GAMEPLAY;
}
