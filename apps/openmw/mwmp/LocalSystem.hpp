#ifndef OPENMW_LOCALSYSTEM_HPP
#define OPENMW_LOCALSYSTEM_HPP

#include <components/openmw-mp/Base/BaseSystem.hpp>
#include <components/openmw-mp/RakNetIncludes.hpp>

namespace mwmp
{
    class Networking;
    class LocalSystem : public BaseSystem
    {
    public:

        LocalSystem();
        virtual ~LocalSystem();

    private:
        Networking *getNetworking();

    };
}

#endif //OPENMW_LOCALSYSTEM_HPP
