#ifndef OPENMW_PACKETMASTERUPDATE_HPP
#define OPENMW_PACKETMASTERUPDATE_HPP

#include "../Packets/BasePacket.hpp"
#include "MasterData.hpp"

namespace mwmp
{
    class ProxyMasterPacket;
    class PacketMasterUpdate : public BasePacket
    {
        friend class ProxyMasterPacket;
    public:
        explicit PacketMasterUpdate(RakNet::RakPeerInterface *peer);

        void Packet(RakNet::BitStream *newBitstream, bool send) override;

        void SetServer(std::pair<RakNet::SystemAddress, QueryData> *serverPair);
    private:
        std::pair<RakNet::SystemAddress, QueryData> *server;
    };
}

#endif //OPENMW_PACKETMASTERUPDATE_HPP
