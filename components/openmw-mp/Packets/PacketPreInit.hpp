#ifndef OPENMW_PACKETPREINIT_HPP
#define OPENMW_PACKETPREINIT_HPP

#include <vector>
#include "BasePacket.hpp"


namespace mwmp
{
    class PacketPreInit : public BasePacket
    {
    public:
        typedef std::vector<uint32_t> HashList;
        typedef std::pair<std::string, HashList> PluginPair;
        typedef std::vector<PluginPair> PluginContainer;

        PacketPreInit(RakNet::RakPeerInterface *peer);

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
        void setChecksums(PluginContainer *checksums);
    private:
        PluginContainer *checksums;
        const static uint32_t maxPlugins = 1000;
        const static uint32_t pluginNameMaxLength = 256;
        const static uint32_t maxHashes = 50;
    };
}


#endif //OPENMW_PACKETPREINIT_HPP
