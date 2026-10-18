#include <components/openmw-mp/NetworkMessages.hpp>
#include "PacketClientScriptLocal.hpp"

using namespace mwmp;

PacketClientScriptLocal::PacketClientScriptLocal(RakNet::RakPeerInterface *peer) : ObjectPacket(peer)
{
    packetID = ID_CLIENT_SCRIPT_LOCAL;
    hasCellData = true;
}

void PacketClientScriptLocal::Object(BaseObject &baseObject, bool send)
{
    ObjectPacket::Object(baseObject, send);

    uint32_t clientLocalsCount;

    if (send)
        clientLocalsCount = static_cast<uint32_t>(baseObject.clientLocals.size());

    RW(clientLocalsCount, send);

    if (!send)
    {
        baseObject.clientLocals.clear();
        baseObject.clientLocals.resize(clientLocalsCount);
    }

    for (auto&& clientLocal : baseObject.clientLocals)
    {
        RW(clientLocal.internalIndex, send);
        RW(clientLocal.variableType, send);

        if (clientLocal.variableType == mwmp::VARIABLE_TYPE::SHORT || clientLocal.variableType == mwmp::VARIABLE_TYPE::LONG)
            RW(clientLocal.intValue, send);
        else if (clientLocal.variableType == mwmp::VARIABLE_TYPE::FLOAT)
            RW(clientLocal.floatValue, send);
    }
}
