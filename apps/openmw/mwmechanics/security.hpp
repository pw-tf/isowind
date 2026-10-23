#ifndef MWMECHANICS_SECURITY_H
#define MWMECHANICS_SECURITY_H

#include <memory>

#include "../mwworld/ptr.hpp"

namespace MWWorld
{
    class Action;
}

namespace MWMechanics
{

    /// @brief implementation of Security skill
    class Security
    {
    public:
        Security(const MWWorld::Ptr& actor);

        void pickLock(const MWWorld::Ptr& lock, const MWWorld::Ptr& lockpick, std::string_view& resultMessage,
            std::string_view& resultSound);
        void probeTrap(const MWWorld::Ptr& trap, const MWWorld::Ptr& probe, std::string_view& resultMessage,
            std::string_view& resultSound);

    private:
        float mAgility, mLuck, mSecuritySkill, mFatigueTerm;
        MWWorld::Ptr mActor;
    };

    /// Isowind: the lockpick the player picks a lock with when they activate it: the one they hold, else the best
    /// one they carry (of equally good ones the most worn). Empty if they carry none with uses left.
    MWWorld::Ptr chooseLockpick(const MWWorld::Ptr& actor);

    /// Isowind: the player activating a locked door or container they have no key to, while carrying a lockpick,
    /// makes one attempt at picking it with chooseLockpick's (Security::pickLock, as attacking the lock with the
    /// lockpick ready does): returns the action that shows the result. nullptr when there is no attempt to make,
    /// and the object is just locked.
    std::unique_ptr<MWWorld::Action> pickLockOnActivation(const MWWorld::Ptr& lock, const MWWorld::Ptr& actor);
}

#endif
