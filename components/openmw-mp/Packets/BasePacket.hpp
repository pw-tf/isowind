#ifndef OPENMW_BASEPACKET_HPP
#define OPENMW_BASEPACKET_HPP

#include <string>
#include <type_traits>
#include <variant>

#include <components/esm/refid.hpp>
#include <components/esm3/refnum.hpp>
#include <components/openmw-mp/Packets/RawWireType.hpp>

#include <components/openmw-mp/RakNetIncludes.hpp>
#include <BitStream.h>
#include <PacketPriority.h>


namespace mwmp
{
    class BasePacket
    {
    public:
        explicit BasePacket(RakNet::RakPeerInterface *peer);

        virtual ~BasePacket() = default;

        virtual void Packet(RakNet::BitStream *newBitstream, bool send);
        virtual uint32_t Send(bool toOtherPlayers = true);
        virtual uint32_t Send(RakNet::AddressOrGUID destination);
        virtual void Read();

        void setGUID(RakNet::RakNetGUID newGuid);
        RakNet::RakNetGUID getGUID();

        void SetReadStream(RakNet::BitStream *bitStream);
        void SetSendStream(RakNet::BitStream *bitStream);
        void SetStreams(RakNet::BitStream *inStream, RakNet::BitStream *outStream);
        virtual uint32_t RequestData(RakNet::RakNetGUID targetGuid);

        static inline uint32_t headerSize()
        {
            return static_cast<uint32_t>(1 + RakNet::RakNetGUID::size()); // packetID + RakNetGUID (uint64_t)
        }

        uint8_t GetPacketID() const
        {
            return packetID;
        }

        bool isPacketValid() const
        {
            return packetValid;
        }

    protected:
        template<class templateType>
        bool RW(templateType &data, uint32_t size, bool write)
        {
            if (write)
                bs->Write(data, size);
            else
                return bs->Read(data, size);
            return true;
        }

        template<class templateType>
        bool RW(templateType &data, bool write, bool compress = 0)
        {
            // Raw copies are only safe for plain values; OpenMW 0.51 changed many record fields (ids especially) to
            // types that must be converted for the wire, so anything else has to get its own overload.
            static_assert(IsRawWireType<templateType>::value,
                "Serialize this type explicitly, or specialize IsRawWireType if it is a plain struct");
            if (write)
            {
                if (compress)
                    bs->WriteCompressed(data);
                else
                    bs->Write(data);
                return true;
            }
            else
            {
                if (compress)
                    return bs->ReadCompressed(data);
                else
                    return bs->Read(data);
            }
        }

        bool RW(bool &data, bool write)
        {
            if (write)
                bs->Write(data);
            else
                return bs->Read(data);
            return true;
        }

        const static uint32_t maxStrSize = 64 * 1024; // 64 KiB

        bool RW(std::string &str, bool write, bool compress = false, std::string::size_type maxSize = maxStrSize)
        {
            bool res = true;
            if (write)
            {
                if (compress)
                    RakNet::RakString::SerializeCompressed(str.substr(0, maxSize).c_str(), bs); // todo: remove extra copy of string
                else
                {
                    RakNet::RakString rstr;
                    rstr.AppendBytes(str.c_str(), str.size() > maxSize ? maxSize : str.size());
                    bs->Write(rstr);
                }
            }
            else
            {
                RakNet::RakString rstr;
                if (compress)
                    res = rstr.DeserializeCompressed(bs);
                else
                    res = bs->Read(rstr);

                if (res)
                {
                    rstr.Truncate(rstr.GetLength() > maxSize ? maxSize : rstr.GetLength());
                    str = rstr.C_String();
                }
                else
                    str = std::string();
            }
            return res;
        }

        // OpenMW 0.51 identifies records with ESM::RefId. On the wire they stay the plain strings TES3MP sends, so the
        // server and its scripts see the same ids. (Without this the generic template above would copy the RefId's
        // bytes, pointers included.)
        bool RW(ESM::RefId &id, bool write, bool compress = false)
        {
            std::string text = write ? id.toString() : std::string();
            bool res = RW(text, write, compress);
            if (!write)
                id = text.empty() ? ESM::RefId() : ESM::RefId::stringRefId(text);
            return res;
        }

        // Magic effects, skills and attributes travel as the int32 indices TES3MP used (the server's scripts work
        // with those numbers); -1 means none.
        bool RWEffectId(ESM::RefId &id, bool write);
        bool RWSkillId(ESM::RefId &id, bool write);
        bool RWAttributeId(ESM::RefId &id, bool write);
        // An active effect's skill or attribute, as the index above. Summons (an actor in 0.51) go as -1, as TES3MP
        // tracked summoned creatures separately.
        bool RWEffectArg(const ESM::RefId &effectId, std::variant<ESM::RefId, ESM::RefNum> &arg, bool write);

    protected:
        uint8_t packetID;
        PacketReliability reliability;
        PacketPriority priority;
        int8_t orderChannel;
        RakNet::BitStream *bsRead, *bsSend, *bs;
        RakNet::RakPeerInterface *peer;
        RakNet::RakNetGUID guid;
        bool packetValid;
    };
}

#endif //OPENMW_BASEPACKET_HPP
