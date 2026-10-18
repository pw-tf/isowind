#ifndef OPENMW_PROCESSOROBJECTHIT_HPP
#define OPENMW_PROCESSOROBJECTHIT_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectHit final: public BaseObjectProcessor
    {
    public:
        ProcessorObjectHit()
        {
            BPP_INIT(ID_OBJECT_HIT)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            //objectList.hitObjects(ptrCellStore);
        }

    };
}

#endif //OPENMW_PROCESSOROBJECTHIT_HPP
