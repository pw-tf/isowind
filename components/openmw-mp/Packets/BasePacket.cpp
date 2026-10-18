#include <components/openmw-mp/NetworkMessages.hpp>
#include <PacketPriority.h>
#include <RakPeer.h>
#include "BasePacket.hpp"

#include <components/esm/attr.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/openmw-mp/EffectArgs.hpp>

using namespace mwmp;

BasePacket::BasePacket(RakNet::RakPeerInterface *peer)
{
    packetID = 0;
    priority = HIGH_PRIORITY;
    reliability = RELIABLE_ORDERED;
    orderChannel = CHANNEL_SYSTEM;
    this->peer = peer;
}

void BasePacket::Packet(RakNet::BitStream *newBitstream, bool send)
{
    bs = newBitstream;
    packetValid = true;

    if (send)
    {
        bs->Write(packetID);
        bs->Write(guid);
    }
}

void BasePacket::SetReadStream(RakNet::BitStream *bitStream)
{
    bsRead = bitStream;
}

void BasePacket::SetSendStream(RakNet::BitStream *bitStream)
{
    bsSend = bitStream;
}

void BasePacket::SetStreams(RakNet::BitStream *inStream, RakNet::BitStream *outStream)
{
    if (inStream != nullptr)
        bsRead = inStream;
    if (outStream != nullptr)
        bsSend = outStream;
}

uint32_t BasePacket::RequestData(RakNet::RakNetGUID targetGuid)
{
    bsSend->ResetWritePointer();
    bsSend->Write(packetID);
    bsSend->Write(targetGuid);
    return peer->Send(bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, orderChannel, targetGuid, false);
}

uint32_t BasePacket::Send(RakNet::AddressOrGUID destination)
{
    bsSend->ResetWritePointer();
    Packet(bsSend, true);
    return peer->Send(bsSend, priority, reliability, orderChannel, destination, false);
}

uint32_t BasePacket::Send(bool toOther)
{
    bsSend->ResetWritePointer();
    Packet(bsSend, true);
    return peer->Send(bsSend, priority, reliability, orderChannel, guid, toOther);
}

void BasePacket::Read()
{
    Packet(bsRead, false);
}

void BasePacket::setGUID(RakNet::RakNetGUID newGuid)
{
    guid = newGuid;
}

RakNet::RakNetGUID BasePacket::getGUID()
{
    return guid;
}

namespace
{
    bool RWIndex(RakNet::BitStream *bs, ESM::RefId &id, bool write, int (*toIndex)(ESM::RefId),
        ESM::RefId (*toRefId)(int))
    {
        int32_t index = write ? (id.empty() ? -1 : toIndex(id)) : -1;
        if (write)
        {
            bs->Write(index);
            return true;
        }
        bool res = bs->Read(index);
        id = res ? toRefId(index) : ESM::RefId();
        return res;
    }
}

bool BasePacket::RWEffectId(ESM::RefId &id, bool write)
{
    return RWIndex(bs, id, write, &ESM::MagicEffect::refIdToIndex, &ESM::MagicEffect::indexToRefId);
}

bool BasePacket::RWSkillId(ESM::RefId &id, bool write)
{
    return RWIndex(bs, id, write, &ESM::Skill::refIdToIndex, &ESM::Skill::indexToRefId);
}

bool BasePacket::RWAttributeId(ESM::RefId &id, bool write)
{
    return RWIndex(bs, id, write, &ESM::Attribute::refIdToIndex, &ESM::Attribute::indexToRefId);
}

bool BasePacket::RWEffectArg(const ESM::RefId &effectId, std::variant<ESM::RefId, ESM::RefNum> &arg, bool write)
{
    int32_t index = write ? effectArgToIndex(arg) : -1;
    bool res = RW(index, write);
    if (!write)
        arg = effectArgFromIndex(effectId, index);
    return res;
}
