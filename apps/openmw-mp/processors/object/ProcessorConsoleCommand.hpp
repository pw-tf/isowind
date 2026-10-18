#ifndef OPENMW_PROCESSORCONSOLECOMMAND_HPP
#define OPENMW_PROCESSORCONSOLECOMMAND_HPP

#include "../ObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorConsoleCommand : public ObjectProcessor
    {
    public:
        ProcessorConsoleCommand()
        {
            BPP_INIT(ID_CONSOLE_COMMAND)
        }

        void Do(ObjectPacket &packet, Player &player, BaseObjectList &objectList) override
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());

            Script::Call<Script::CallbackIdentity("OnConsoleCommand")>(player.getId(), objectList.cell.getShortDescription().c_str());
        }
    };
}

#endif //OPENMW_PROCESSORCONSOLECOMMAND_HPP
