#include "PacketGameSettings.hpp"
#include <components/openmw-mp/NetworkMessages.hpp>

using namespace mwmp;

PacketGameSettings::PacketGameSettings(RakNet::RakPeerInterface *peer) : PlayerPacket(peer)
{
    packetID = ID_GAME_SETTINGS;
    orderChannel = CHANNEL_SYSTEM;
}

void PacketGameSettings::Packet(RakNet::BitStream *newBitstream, bool send)
{
    PlayerPacket::Packet(newBitstream, send);

    RW(player->difficulty, send);
    RW(player->consoleAllowed, send);
    RW(player->bedRestAllowed, send);
    RW(player->wildernessRestAllowed, send);
    RW(player->waitAllowed, send);
    RW(player->enforcedLogLevel, send);
    RW(player->physicsFramerate, send);

    std::string mapIndex;
    std::string mapValue;

    uint32_t gameSettingCount = static_cast<uint32_t>(player->gameSettings.size());
    RW(gameSettingCount, send);

    if (send)
    {
        for (auto&& gameSetting : player->gameSettings)
        {
            mapIndex = gameSetting.first;
            mapValue = gameSetting.second;
            RW(mapIndex, send, false);
            RW(mapValue, send, false);
        }
    }
    else
    {
        player->gameSettings.clear();
        for (unsigned int n = 0; n < gameSettingCount; n++)
        {
            RW(mapIndex, send, false);
            RW(mapValue, send, false);
            player->gameSettings[mapIndex] = mapValue;
        }
    }

    uint32_t vrSettingCount = static_cast<uint32_t>(player->vrSettings.size());
    RW(vrSettingCount, send);

    if (send)
    {
        for (auto&& vrSetting : player->vrSettings)
        {
            mapIndex = vrSetting.first;
            mapValue = vrSetting.second;
            RW(mapIndex, send, false);
            RW(mapValue, send, false);
        }
    }
    else
    {
        player->vrSettings.clear();
        for (unsigned int n = 0; n < vrSettingCount; n++)
        {
            RW(mapIndex, send, false);
            RW(mapValue, send, false);
            player->vrSettings[mapIndex] = mapValue;
        }
    }
}
