#ifndef OPENMW_MP_EFFECTARGS_HPP
#define OPENMW_MP_EFFECTARGS_HPP

#include <variant>

#include <components/esm/refid.hpp>
#include <components/esm3/refnum.hpp>

namespace mwmp
{
    // An active effect's argument as TES3MP numbers it: the skill or attribute index, or -1 (none, or a summon, which
    // OpenMW 0.51 stores as the summoned actor while TES3MP tracked summons separately).
    int effectArgToIndex(const std::variant<ESM::RefId, ESM::RefNum> &arg);

    // Back again; which kind of index it is depends on the effect.
    std::variant<ESM::RefId, ESM::RefNum> effectArgFromIndex(const ESM::RefId &effectId, int index);
}

#endif //OPENMW_MP_EFFECTARGS_HPP
