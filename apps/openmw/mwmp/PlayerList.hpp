#ifndef OPENMW_PLAYERLIST_HPP
#define OPENMW_PLAYERLIST_HPP

#include <components/esm3/custommarkerstate.hpp>
#include <components/esm3/loadnpc.hpp>
#include <components/openmw-mp/Base/BasePlayer.hpp>

#include "../mwmechanics/aisequence.hpp"

#include "../mwworld/manualref.hpp"

#include "DedicatedPlayer.hpp"

#include <map>
#include <components/openmw-mp/RakNetIncludes.hpp>

namespace MWMechanics
{
    class Actor;
}

namespace mwmp
{
    class PlayerList
    {
    public:

        static void update(float dt);

        static DedicatedPlayer *newPlayer(RakNet::RakNetGUID guid);

        static void deletePlayer(RakNet::RakNetGUID guid);
        static void cleanUp();

        static DedicatedPlayer *getPlayer(RakNet::RakNetGUID guid);
        static DedicatedPlayer *getPlayer(const MWWorld::Ptr &ptr);
        static DedicatedPlayer* getPlayer(ESM::RefNum actor);
        static std::vector<RakNet::RakNetGUID> getPlayersInCell(const ESM::Cell& cell);
        static std::vector<DedicatedPlayer*> getPlayers();

        static bool isDedicatedPlayer(const MWWorld::Ptr &ptr);

        static void enableMarkers(const ESM::Cell& cell);

        static void clearHitAttemptActor(ESM::RefNum actor);

    private:

        static std::map<RakNet::RakNetGUID, DedicatedPlayer *> playerList;
    };
}

#endif //OPENMW_PLAYERLIST_HPP
