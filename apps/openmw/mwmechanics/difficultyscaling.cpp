#include "difficultyscaling.hpp"

#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwmp/LocalPlayer.hpp"
#include "../mwmp/Main.hpp"

#include "actorutil.hpp"

float scaleDamage(float damage, const MWWorld::Ptr& attacker, const MWWorld::Ptr& victim)
{
    const MWWorld::Ptr& player = MWMechanics::getPlayer();

    static const float fDifficultyMult
        = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>().find("fDifficultyMult")->mValue.getFloat();

    // Isowind multiplayer: the server sets the difficulty
    const int difficulty = mwmp::Main::isInitialized() ? mwmp::Main::get().getLocalPlayer()->difficulty
                                                        : static_cast<int>(Settings::game().mDifficulty);
    const float difficultyTerm = 0.01f * difficulty;

    float x = 0;
    if (victim == player)
    {
        if (difficultyTerm > 0)
            x = fDifficultyMult * difficultyTerm;
        else
            x = difficultyTerm / fDifficultyMult;
    }
    else if (attacker == player)
    {
        if (difficultyTerm > 0)
            x = -difficultyTerm / fDifficultyMult;
        else
            x = fDifficultyMult * (-difficultyTerm);
    }

    damage *= 1 + x;
    return damage;
}
