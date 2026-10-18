#include "actiontalk.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmp/Hooks.hpp"

#include "../mwmechanics/actorutil.hpp"

namespace MWWorld
{
    ActionTalk::ActionTalk(const Ptr& actor)
        : Action(false, actor)
    {
    }

    void ActionTalk::executeImp(const Ptr& actor)
    {
        // (Isowind multiplayer: the server can also have an NPC start a conversation with the player)
        if (actor == MWMechanics::getPlayer() || mwmp::hooks::isMultiplayer())
            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_Dialogue, getTarget());
    }
}
