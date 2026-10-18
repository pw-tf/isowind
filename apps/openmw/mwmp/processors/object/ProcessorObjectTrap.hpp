#ifndef OPENMW_PROCESSOROBJECTTRAP_HPP
#define OPENMW_PROCESSOROBJECTTRAP_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectTrap final: public BaseObjectProcessor
    {
    public:
        ProcessorObjectTrap()
        {
            BPP_INIT(ID_OBJECT_TRAP)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            objectList.triggerTrapObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTTRAP_HPP
