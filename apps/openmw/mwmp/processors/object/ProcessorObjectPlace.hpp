#ifndef OPENMW_PROCESSOROBJECTPLACE_HPP
#define OPENMW_PROCESSOROBJECTPLACE_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectPlace final: public BaseObjectProcessor
    {
    public:
        ProcessorObjectPlace()
        {
            BPP_INIT(ID_OBJECT_PLACE)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            objectList.placeObjects(ptrCellStore);
        }

    };
}

#endif //OPENMW_PROCESSOROBJECTPLACE_HPP
