#ifndef OPENMW_PROCESSORPLAYERCELLCHANGE_HPP
#define OPENMW_PROCESSORPLAYERCELLCHANGE_HPP

#include "../PlayerProcessor.hpp"
#include "apps/openmw-mp/Networking.hpp"
#include "apps/openmw-mp/Script/Script.hpp"
#include <components/openmw-mp/Controllers/PlayerPacketController.hpp>

namespace mwmp
{
    class ProcessorPlayerCellChange : public PlayerProcessor
    {
        PlayerPacketController *playerController;
    public:
        ProcessorPlayerCellChange()
        {
            BPP_INIT(ID_PLAYER_CELL_CHANGE)
            playerController = Networking::get().getPlayerPacketController();
        }

        void Do(PlayerPacket &packet, Player &player) override
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received %s from %s", strPacketID.c_str(), player.npc.mName.c_str());
            LOG_APPEND(TimedLog::LOG_INFO, "- Moved to %s", player.cell.getShortDescription().c_str());

            Script::Call<Script::CallbackIdentity("OnPlayerCellChange")>(player.getId());

            player.exchangeFullInfo = true;

            player.forEachLoaded([this](Player *pl, Player *other) {

                LOG_APPEND(TimedLog::LOG_INFO, "- Started information exchange with %s", other->npc.mName.c_str());

                other->exchangeFullInfo = true;

                playerController->GetPacket(ID_PLAYER_STATS_DYNAMIC)->setPlayer(other);
                playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->setPlayer(other);
                playerController->GetPacket(ID_PLAYER_POSITION)->setPlayer(other);
                playerController->GetPacket(ID_PLAYER_SKILL)->setPlayer(other);
                playerController->GetPacket(ID_PLAYER_EQUIPMENT)->setPlayer(other);
                playerController->GetPacket(ID_PLAYER_ANIM_FLAGS)->setPlayer(other);
                playerController->GetPacket(ID_PLAYER_SHAPESHIFT)->setPlayer(other);

                playerController->GetPacket(ID_PLAYER_STATS_DYNAMIC)->Send(pl->guid);
                playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->Send(pl->guid);
                playerController->GetPacket(ID_PLAYER_POSITION)->Send(pl->guid);
                playerController->GetPacket(ID_PLAYER_SKILL)->Send(pl->guid);
                playerController->GetPacket(ID_PLAYER_EQUIPMENT)->Send(pl->guid);
                playerController->GetPacket(ID_PLAYER_ANIM_FLAGS)->Send(pl->guid);
                playerController->GetPacket(ID_PLAYER_SHAPESHIFT)->Send(pl->guid);

                playerController->GetPacket(ID_PLAYER_STATS_DYNAMIC)->setPlayer(pl);
                playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->setPlayer(pl);
                playerController->GetPacket(ID_PLAYER_SKILL)->setPlayer(pl);
                playerController->GetPacket(ID_PLAYER_EQUIPMENT)->setPlayer(pl);
                playerController->GetPacket(ID_PLAYER_ANIM_FLAGS)->setPlayer(pl);
                playerController->GetPacket(ID_PLAYER_SHAPESHIFT)->setPlayer(pl);

                playerController->GetPacket(ID_PLAYER_STATS_DYNAMIC)->Send(other->guid);
                playerController->GetPacket(ID_PLAYER_ATTRIBUTE)->Send(other->guid);
                playerController->GetPacket(ID_PLAYER_SKILL)->Send(other->guid);
                playerController->GetPacket(ID_PLAYER_EQUIPMENT)->Send(other->guid);
                playerController->GetPacket(ID_PLAYER_ANIM_FLAGS)->Send(other->guid);
                playerController->GetPacket(ID_PLAYER_SHAPESHIFT)->Send(other->guid);

                other->exchangeFullInfo = false;

                LOG_APPEND(TimedLog::LOG_INFO, "- Finished information exchange with %s", other->npc.mName.c_str());
            });

            playerController->GetPacket(ID_PLAYER_POSITION)->setPlayer(&player);
            playerController->GetPacket(ID_PLAYER_POSITION)->Send();
            packet.setPlayer(&player);
            packet.Send(true); //send to other clients

            LOG_APPEND(TimedLog::LOG_INFO, "- Finished processing ID_PLAYER_CELL_CHANGE", player.cell.getShortDescription().c_str());

            player.exchangeFullInfo = false;
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERCELLCHANGE_HPP
