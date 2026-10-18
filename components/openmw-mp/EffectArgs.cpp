#include "EffectArgs.hpp"

#include <algorithm>
#include <array>

#include <components/esm/attr.hpp>
#include <components/esm3/loadmgef.hpp>
#include <components/esm3/loadskil.hpp>

namespace
{
    bool affectsAttribute(const ESM::RefId &effectId)
    {
        static const std::array effects{ ESM::MagicEffect::DrainAttribute, ESM::MagicEffect::DamageAttribute,
            ESM::MagicEffect::RestoreAttribute, ESM::MagicEffect::FortifyAttribute,
            ESM::MagicEffect::AbsorbAttribute };
        return std::find(effects.begin(), effects.end(), effectId) != effects.end();
    }

    bool affectsSkill(const ESM::RefId &effectId)
    {
        static const std::array effects{ ESM::MagicEffect::DrainSkill, ESM::MagicEffect::DamageSkill,
            ESM::MagicEffect::RestoreSkill, ESM::MagicEffect::FortifySkill, ESM::MagicEffect::AbsorbSkill };
        return std::find(effects.begin(), effects.end(), effectId) != effects.end();
    }
}

int mwmp::effectArgToIndex(const std::variant<ESM::RefId, ESM::RefNum> &arg)
{
    const ESM::RefId *id = std::get_if<ESM::RefId>(&arg);
    if (!id || id->empty())
        return -1;
    int index = ESM::Skill::refIdToIndex(*id);
    return index >= 0 ? index : ESM::Attribute::refIdToIndex(*id);
}

std::variant<ESM::RefId, ESM::RefNum> mwmp::effectArgFromIndex(const ESM::RefId &effectId, int index)
{
    if (affectsAttribute(effectId))
        return ESM::Attribute::indexToRefId(index);
    if (affectsSkill(effectId))
        return ESM::Skill::indexToRefId(index);
    return ESM::RefId();
}
