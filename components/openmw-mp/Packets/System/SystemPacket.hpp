#ifndef OPENMW_SYSTEMPACKET_HPP
#define OPENMW_SYSTEMPACKET_HPP

#include <string>
#include <components/openmw-mp/RakNetIncludes.hpp>
#include <BitStream.h>
#include <PacketPriority.h>
#include <components/openmw-mp/Base/BaseSystem.hpp>

#include <components/openmw-mp/Packets/BasePacket.hpp>

namespace mwmp
{
    class SystemPacket : public BasePacket
    {
    public:
        SystemPacket(RakNet::RakPeerInterface *peer);

        ~SystemPacket();

        void setSystem(BaseSystem *newSystem);
        BaseSystem *getSystem();

    protected:
        BaseSystem *system;

    };
}

#endif //OPENMW_SYSTEMPACKET_HPP
