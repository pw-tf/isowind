#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/TimedLog.hpp>
#include "PacketActorTest.hpp"

using namespace mwmp;

PacketActorTest::PacketActorTest(RakNet::RakPeerInterface *peer) : ActorPacket(peer)
{
    packetID = ID_ACTOR_TEST;
}

void PacketActorTest::Actor(BaseActor &actor, bool send)
{

}
