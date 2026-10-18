#ifndef OPENMW_PROCESSDOORDESTINATION_HPP
#define OPENMW_PROCESSDOORDESTINATION_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorDoorDestination final: public BaseObjectProcessor
    {
    public:
        ProcessorDoorDestination()
        {
            BPP_INIT(ID_DOOR_DESTINATION)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            objectList.setDoorDestinations(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSDOORDESTINATION_HPP
