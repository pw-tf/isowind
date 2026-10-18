#ifndef OPENMW_MWMP_IDS_HPP
#define OPENMW_MWMP_IDS_HPP

#include <string>
#include <string_view>

#include <components/esm/attr.hpp>
#include <components/esm/refid.hpp>
#include <components/esm3/loadskil.hpp>

namespace mwmp
{
    // Multiplayer data keeps ids as the strings TES3MP sends; OpenMW 0.51 uses ESM::RefId. These convert between the
    // two where the multiplayer code meets the engine.

    inline std::string idString(const ESM::RefId &id)
    {
        return id.toString();
    }

    inline ESM::RefId refId(std::string_view id)
    {
        return ESM::RefId::stringRefId(id);
    }

    // Whether an id is the one a string names (case-insensitively); an empty string names the empty id.
    inline bool isSameId(const ESM::RefId &id, std::string_view text)
    {
        return text.empty() ? id.empty() : id == text;
    }

    // Ids only this client knows (records the game generated, like a spell the player made): the server can't
    // identify them, so TES3MP never sent them ("$dynamic" ids there).
    inline bool isLocalOnlyId(const ESM::RefId &id)
    {
        return !id.empty() && !id.is<ESM::StringRefId>();
    }

    // TES3MP numbers attributes and skills; 0.51 names them.
    inline ESM::Attribute::AttributeID attributeId(int index)
    {
        return *ESM::Attribute::indexToRefId(index).getIf<ESM::StringRefId>();
    }

    inline ESM::RefId skillId(int index)
    {
        return ESM::Skill::indexToRefId(index);
    }
}

#endif //OPENMW_MWMP_IDS_HPP
