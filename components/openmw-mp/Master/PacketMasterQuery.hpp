#ifndef OPENMW_PACKETMASTERQUERY_HPP
#define OPENMW_PACKETMASTERQUERY_HPP

#include "../Packets/BasePacket.hpp"
#include "MasterData.hpp"

namespace mwmp
{
    class ProxyMasterPacket;
    class PacketMasterQuery : public BasePacket
    {
        friend class ProxyMasterPacket;
    public:
        explicit PacketMasterQuery(RakNet::RakPeerInterface *peer);

        void Packet(RakNet::BitStream *newBitstream, bool send) override;

        void SetServers(std::map<RakNet::SystemAddress, QueryData> *serverMap);
    private:
        std::map<RakNet::SystemAddress, QueryData> *servers;
    };
}

#endif //OPENMW_PACKETMASTERQUERY_HPP
