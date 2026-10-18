#ifndef OPENMW_PACKETRECORDDYNAMIC_HPP
#define OPENMW_PACKETRECORDDYNAMIC_HPP

#include <components/openmw-mp/Packets/Worldstate/WorldstatePacket.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>

namespace mwmp
{
    class PacketRecordDynamic: public WorldstatePacket
    {
    public:
        PacketRecordDynamic(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);

        void ProcessEffects(ESM::EffectList &effectList, bool send);
        void ProcessBodyParts(ESM::PartReferenceList &bodyPartList, bool send);
        void ProcessInventoryList(std::vector<mwmp::Item> &inventory, ESM::InventoryList &inventoryList, bool send);

    protected:
        static const int maxRecords = 3000;
        static const int maxEffects = 100;
        static const int maxParts = 7;
        static const int maxItems = 1000;
    };
}

#endif //OPENMW_PACKETRECORDDYNAMIC_HPP
