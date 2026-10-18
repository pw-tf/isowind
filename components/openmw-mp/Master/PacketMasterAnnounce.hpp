#ifndef OPENMW_PACKETMASTERANNOUNCE_HPP
#define OPENMW_PACKETMASTERANNOUNCE_HPP

#include "../Packets/BasePacket.hpp"
#include "MasterData.hpp"

namespace mwmp
{
    class ProxyMasterPacket;
    class PacketMasterAnnounce : public BasePacket
    {
        friend class ProxyMasterPacket;
    public:
        explicit PacketMasterAnnounce(RakNet::RakPeerInterface *peer);

        void Packet(RakNet::BitStream *newBitstream, bool send) override;

        void SetServer(QueryData *server);
        void SetFunc(uint32_t keep);
        int GetFunc();

        enum Func
        {
            FUNCTION_DELETE = 0,
            FUNCTION_ANNOUNCE,
            FUNCTION_KEEP
        };
    private:
        QueryData *server;
        uint32_t func;
    };
}

#endif //OPENMW_PACKETMASTERANNOUNCE_HPP
