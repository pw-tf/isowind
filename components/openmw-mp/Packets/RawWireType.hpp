#ifndef OPENMW_RAWWIRETYPE_HPP
#define OPENMW_RAWWIRETYPE_HPP

#include <array>
#include <cstddef>
#include <type_traits>

#include <components/openmw-mp/RakNetIncludes.hpp>

namespace mwmp
{
    // Types the generic RW below may copy byte for byte. Plain values are; a packet that sends a plain struct (no ids,
    // pointers or containers inside) specializes this for it where the struct is declared, so each raw struct is a reviewed choice.
    template <class T>
    struct IsRawWireType : std::bool_constant<std::is_arithmetic_v<T> || std::is_enum_v<T>
                               || std::is_same_v<T, RakNet::RakNetGUID> || std::is_same_v<T, RakNet::SystemAddress>>
    {
    };
    template <class T, std::size_t N>
    struct IsRawWireType<T[N]> : IsRawWireType<T>
    {
    };
    template <class T, std::size_t N>
    struct IsRawWireType<std::array<T, N>> : IsRawWireType<T>
    {
    };
}

#endif //OPENMW_RAWWIRETYPE_HPP
