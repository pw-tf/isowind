#include <components/openmw-mp/NetworkMessages.hpp>
#include <PacketPriority.h>
#include <RakPeer.h>
#include "SystemPacket.hpp"

using namespace mwmp;

SystemPacket::SystemPacket(RakNet::RakPeerInterface *peer) : BasePacket(peer)
{
    packetID = 0;
    priority = HIGH_PRIORITY;
    reliability = RELIABLE_ORDERED;
    orderChannel = CHANNEL_SYSTEM;
    this->peer = peer;
}

SystemPacket::~SystemPacket()
{

}

void SystemPacket::setSystem(BaseSystem *newSystem)
{
    system = newSystem;
    guid = system->guid;
}

BaseSystem *SystemPacket::getSystem()
{
    return system;
}
