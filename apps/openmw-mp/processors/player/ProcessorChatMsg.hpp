#ifndef OPENMW_PROCESSORPLAYERCHATMSG_HPP
#define OPENMW_PROCESSORPLAYERCHATMSG_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorChatMsg : public PlayerProcessor
    {
    public:
        ProcessorChatMsg()
        {
            BPP_INIT(ID_CHAT_MESSAGE)
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            DEBUG_PRINTF(strPacketID.c_str());

            Script::Call<Script::CallbackIdentity("OnPlayerSendMessage")>(player.getId(), player.chatMessage.c_str());
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERCHATMSG_HPP
