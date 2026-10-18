//
// Created by koncord on 22.04.17.
//

#ifndef OPENMW_PROXYMASTERPACKET_HPP
#define OPENMW_PROXYMASTERPACKET_HPP

#include <components/openmw-mp/Packets/BasePacket.hpp>
#include "MasterData.hpp"
#include <iostream>

namespace mwmp
{
    class ProxyMasterPacket : public BasePacket
    {
    private:
        explicit ProxyMasterPacket(RakNet::RakPeerInterface *peer) : BasePacket(peer)
        {
        }

    public:
        template<class Packet>
        static void addServer(Packet *packet, QueryData &server, bool send)
        {
            int32_t rulesSize = server.rules.size();
            packet->RW(rulesSize, send);

            if (rulesSize > QueryData::maxRules)
                rulesSize = 0;

            std::map<std::string, ServerRule>::iterator ruleIt;
            if (send)
                ruleIt = server.rules.begin();

            while (rulesSize--)
            {
                ServerRule *rule = nullptr;
                std::string key;
                if (send)
                {
                    key = ruleIt->first;
                    rule = &ruleIt->second;
                }

                packet->RW(key, send, false, QueryData::maxStringLength);
                if (!send)
                {
                    ruleIt = server.rules.insert(std::pair<std::string, ServerRule>(key, ServerRule())).first;
                    rule = &ruleIt->second;
                }

                packet->RW(rule->type, send);

                if (rule->type == ServerRule::Type::string)
                    packet->RW(rule->str, send, QueryData::maxStringLength);
                else
                    packet->RW(rule->val, send);

                if (send)
                    ruleIt++;
            }

            std::vector<std::string>::iterator plIt;

            int32_t playersCount = server.players.size();
            packet->RW(playersCount, send);

            if (playersCount > QueryData::maxPlayers)
                playersCount = 0;

            if (!send)
            {
                server.players.clear();
                server.players.resize(playersCount);
            }

            for(auto &&player : server.players)
                packet->RW(player, send, false, QueryData::maxStringLength);


            int32_t pluginsCount = server.plugins.size();
            packet->RW(pluginsCount, send);

            if (pluginsCount > QueryData::maxPlugins)
                pluginsCount = 0;

            if (!send)
            {
                server.plugins.clear();
                server.plugins.resize(pluginsCount);
            }

            for (auto &&plugin : server.plugins)
            {
                packet->RW(plugin.name, send, false, QueryData::maxStringLength);
                packet->RW(plugin.hash, send);
            }
        }
    };
}

#endif //OPENMW_PROXYMASTERPACKET_HPP
