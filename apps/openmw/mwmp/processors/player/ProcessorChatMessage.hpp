#ifndef OPENMW_PROCESSORCHATMESSAGE_HPP
#define OPENMW_PROCESSORCHATMESSAGE_HPP


#include "../PlayerProcessor.hpp"
#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/GUIController.hpp"

namespace mwmp
{
    class ProcessorChatMessage final: public PlayerProcessor
    {
    public:
        ProcessorChatMessage()
        {
            BPP_INIT(ID_CHAT_MESSAGE)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            if (player != 0)
                Main::get().getGUIController()->printChatMessage(player->chatMessage);
        }
    };
}

#endif //OPENMW_PROCESSORCHATMESSAGE_HPP
