#ifndef OPENMW_ACTORPROCESSOR_HPP
#define OPENMW_ACTORPROCESSOR_HPP


#include <components/openmw-mp/Base/BasePacketProcessor.hpp>
#include <components/openmw-mp/Packets/BasePacket.hpp>
#include <components/openmw-mp/Packets/Actor/ActorPacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include "Script/Script.hpp"
#include "Player.hpp"

namespace mwmp
{
    class ActorProcessor : public BasePacketProcessor<ActorProcessor>
    {
    public:

        virtual void Do(ActorPacket &packet, Player &player, BaseActorList &actorList);

        static bool Process(RakNet::Packet &packet, BaseActorList &actorList) noexcept;

        // Isowind MMO: only the copies of the player's own followers (FOLLOWER_COPY_BASE), which their game moves
        // whoever has authority over the cell
        static bool isOwnFollowerCopies(Player &player, const BaseActorList &actorList)
        {
            const unsigned int first = FOLLOWER_COPY_BASE + player.getId() * FOLLOWER_COPIES_PER_PLAYER;
            if (actorList.baseActors.empty())
                return false;
            for (const auto &actor : actorList.baseActors)
                if (actor.refNum != 0 || actor.mpNum < first || actor.mpNum >= first + FOLLOWER_COPIES_PER_PLAYER)
                    return false;
            return true;
        }
    };
}

#endif //OPENMW_ACTORPROCESSOR_HPP
