#ifndef OPENMW_BASEACTOR_HPP
#define OPENMW_BASEACTOR_HPP

#include <components/esm3/loadcell.hpp>

#include <components/openmw-mp/Base/BaseStructs.hpp>

#include <components/openmw-mp/RakNetIncludes.hpp>

namespace mwmp
{
    // Isowind MMO: a player's own follower as the other players see it (a look-alike their games make,
    // MechanicsHelper::isFollowerCopy): numbered 0-<mpNum>, the mpNum being this base plus its owner's player id times
    // FOLLOWER_COPIES_PER_PLAYER plus which of their followers it is. Only the owner's game moves it.
    const unsigned int FOLLOWER_COPY_BASE = 0x40000000;
    const unsigned int FOLLOWER_COPIES_PER_PLAYER = 256;

    class BaseActor
    {
    public:

        BaseActor()
        {
            hasPositionData = false;
            hasStatsDynamicData = false;
        }

        std::string refId = "";
        unsigned int refNum;
        unsigned int mpNum;

        ESM::Position position;
        ESM::Position direction;

        ESM::Cell cell;

        unsigned int movementFlags;
        char drawState;
        bool isFlying;

        std::string sound;

        SimpleCreatureStats creatureStats;

        Animation animation;
        char deathState;
        bool isInstantDeath = false;
        Attack attack;
        Cast cast;

        Target killer;

        bool isFollowerCellChange = false;

        bool hasAiTarget = false;
        Target aiTarget;
        unsigned int aiAction = 0;
        unsigned int aiDistance = 0;
        unsigned int aiDuration = 0;
        bool aiShouldRepeat = false;
        ESM::Position aiCoordinates;

        bool hasPositionData;
        bool hasStatsDynamicData;

        Item equipmentItems[19];
        SpellsActiveChanges spellsActiveChanges;
    };

    class BaseActorList
    {
    public:

        BaseActorList()
        {

        }

        enum ACTOR_ACTION
        {
            SET = 0,
            ADD = 1,
            REMOVE = 2,
            REQUEST = 3
        };

        enum AI_ACTION
        {
            CANCEL = 0,
            ACTIVATE = 1,
            COMBAT = 2,
            ESCORT = 3,
            FOLLOW = 4,
            TRAVEL = 5,
            WANDER = 6
        };

        RakNet::RakNetGUID guid;

        std::vector<BaseActor> baseActors;

        unsigned int count;

        ESM::Cell cell;

        unsigned char action; // 0 - Clear and set in entirety, 1 - Add item, 2 - Remove item, 3 - Request items

        bool isValid;
    };
}

#endif //OPENMW_BASEACTOR_HPP
