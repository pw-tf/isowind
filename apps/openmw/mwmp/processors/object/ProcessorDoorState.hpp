#ifndef OPENMW_PROCESSDOORSTATE_HPP
#define OPENMW_PROCESSDOORSTATE_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorDoorState final: public BaseObjectProcessor
    {
    public:
        ProcessorDoorState()
        {
            BPP_INIT(ID_DOOR_STATE)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            objectList.activateDoors(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSDOORSTATE_HPP
