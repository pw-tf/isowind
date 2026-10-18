#ifndef OPENMW_BASEWORLDSTATEPROCESSOR_HPP
#define OPENMW_BASEWORLDSTATEPROCESSOR_HPP

#include <components/openmw-mp/Base/BasePacketProcessor.hpp>
#include <components/openmw-mp/Packets/BasePacket.hpp>
#include <components/openmw-mp/Packets/Worldstate/WorldstatePacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include "Player.hpp"

namespace mwmp
{
    class WorldstateProcessor : public BasePacketProcessor<WorldstateProcessor>
    {
    public:

        virtual void Do(WorldstatePacket &packet, Player &player, BaseWorldstate &worldstate);

        static bool Process(RakNet::Packet &packet, BaseWorldstate &worldstate) noexcept;
    };
}

#endif //OPENMW_BASEWORLDSTATEPROCESSOR_HPP
