#ifndef OPENMW_NETWORKING_HPP
#define OPENMW_NETWORKING_HPP

#include <RakPeerInterface.h>
#include <BitStream.h>
#include <string>

#include <components/openmw-mp/NetworkMessages.hpp>

#include <components/openmw-mp/Controllers/SystemPacketController.hpp>
#include <components/openmw-mp/Controllers/PlayerPacketController.hpp>
#include <components/openmw-mp/Controllers/ActorPacketController.hpp>
#include <components/openmw-mp/Controllers/ObjectPacketController.hpp>
#include <components/openmw-mp/Controllers/WorldstatePacketController.hpp>

#include <components/files/collections.hpp>

#include "LocalSystem.hpp"
#include "ActorList.hpp"
#include "ObjectList.hpp"
#include "Worldstate.hpp"

namespace mwmp
{
    class LocalPlayer;

    class Networking
    {
    public:
        Networking();
        ~Networking();
        void connect(const std::string& ip, unsigned short port, const std::vector<std::string> &content, const Files::Collections &collections);
        void update();

        SystemPacket *getSystemPacket(RakNet::MessageID id);
        PlayerPacket *getPlayerPacket(RakNet::MessageID id);
        ActorPacket *getActorPacket(RakNet::MessageID id);
        ObjectPacket *getObjectPacket(RakNet::MessageID id);
        WorldstatePacket *getWorldstatePacket(RakNet::MessageID id);

        RakNet::SystemAddress serverAddress()
        {
            return serverAddr;
        }

        bool isConnected();

        LocalSystem *getLocalSystem();
        LocalPlayer *getLocalPlayer();
        ActorList *getActorList();
        ObjectList *getObjectList();
        Worldstate *getWorldstate();

    private:
        bool connected;
        RakNet::RakPeerInterface *peer;
        RakNet::SystemAddress serverAddr;
        RakNet::BitStream bsOut;

        SystemPacketController systemPacketController;
        PlayerPacketController playerPacketController;
        ActorPacketController actorPacketController;
        ObjectPacketController objectPacketController;
        WorldstatePacketController worldstatePacketController;

        ActorList actorList;
        ObjectList objectList;
        Worldstate worldstate;

        void receiveMessage(RakNet::Packet *packet);

        void preInit(const std::vector<std::string> &content, const Files::Collections &collections);
    };
}


#endif //OPENMW_NETWORKING_HPP
