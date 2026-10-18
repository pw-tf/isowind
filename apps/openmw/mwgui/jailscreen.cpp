#include <MyGUI_ScrollBar.h>

#include <components/misc/rng.hpp>
#include <components/misc/strings/format.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/luamanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwmp/LocalPlayer.hpp"
#include "../mwmp/Main.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/store.hpp"

#include "jailscreen.hpp"

namespace MWGui
{
    JailScreen::JailScreen()
        : WindowBase("openmw_jail_screen.layout")
        , mDays(1)
        , mFadeTimeRemaining(0)
    {
        getWidget(mProgressBar, "ProgressBar");

        mTimeAdvancer.eventProgressChanged += MyGUI::newDelegate(this, &JailScreen::onJailProgressChanged);
        mTimeAdvancer.eventFinished += MyGUI::newDelegate(this, &JailScreen::onJailFinished);

        center();
    }

    void JailScreen::goToJail(int days)
    {
        mDays = days;

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.5);
        mFadeTimeRemaining = 0.5;

        setVisible(false);
        mProgressBar->setScrollRange(100 + 1);
        mProgressBar->setScrollPosition(0);
        mProgressBar->setTrackSize(0);

        // Isowind multiplayer: the server can replace the text shown while in prison
        const bool customText
            = mwmp::Main::isInitialized() && !mwmp::Main::get().getLocalPlayer()->jailProgressText.empty();
        setText("LoadingText", customText ? mwmp::Main::get().getLocalPlayer()->jailProgressText : "#{sInPrisonTitle}");
    }

    void JailScreen::onFrame(float dt)
    {
        mTimeAdvancer.onFrame(dt);

        if (mFadeTimeRemaining <= 0)
            return;

        mFadeTimeRemaining -= dt;

        if (mFadeTimeRemaining <= 0)
        {
            MWWorld::Ptr player = MWMechanics::getPlayer();
            // (In multiplayer the server can keep the player where they are)
            if (!mwmp::Main::isInitialized() || !mwmp::Main::get().getLocalPlayer()->ignoreJailTeleportation)
            {
                MWBase::Environment::get().getWorld()->teleportToClosestMarker(
                    player, ESM::RefId::stringRefId("prisonmarker"));
                MWBase::Environment::get().getWindowManager()->fadeScreenOut(
                    0.f); // override fade-in caused by cell transition
            }

            setVisible(true);
            mTimeAdvancer.run(100);
        }
    }

    void JailScreen::onJailProgressChanged(int cur, int /*total*/)
    {
        mProgressBar->setScrollPosition(0);
        mProgressBar->setTrackSize(
            static_cast<int>(cur / (float)(mProgressBar->getScrollRange()) * mProgressBar->getLineSize()));
    }

    void JailScreen::onJailFinished()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Jail);
        MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);

        MWWorld::Ptr player = MWMechanics::getPlayer();

        MWBase::Environment::get().getMechanicsManager()->rest(mDays * 24, true);
        // In multiplayer the server keeps the time
        if (!mwmp::Main::isInitialized())
            MWBase::Environment::get().getWorld()->advanceTime(mDays * 24);

        // We should not worsen corprus when in prison
        player.getClass().getCreatureStats(player).getActiveSpells().skipWorsenings(mDays * 24);

        if (!mwmp::Main::isInitialized())
        {
            MWBase::Environment::get().getLuaManager()->jailTimeServed(player, mDays);
            return;
        }

        // Isowind multiplayer: the server can leave skills alone, and replace the message on release
        mwmp::LocalPlayer* localPlayer = mwmp::Main::get().getLocalPlayer();
        const std::string endText = localPlayer->jailEndText;
        if (localPlayer->ignoreJailSkillIncreases)
        {
            const MWWorld::Store<ESM::GameSetting>& gmst
                = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
            const std::string message = endText.empty()
                ? Misc::StringUtils::format(
                      gmst.find(mDays == 1 ? "sNotifyMessage42" : "sNotifyMessage43")->mValue.getString(), mDays)
                : endText;
            MWBase::Environment::get().getWindowManager()->interactiveMessageBox(message, { "#{Interface:OK}" });
        }
        else
        {
            MWBase::Environment::get().getLuaManager()->jailTimeServed(player, mDays);
            if (!endText.empty())
                MWBase::Environment::get().getWindowManager()->messageBox(endText);
        }

        localPlayer->ignoreJailTeleportation = false;
        localPlayer->ignoreJailSkillIncreases = false;
        localPlayer->jailProgressText.clear();
        localPlayer->jailEndText.clear();
    }
}
