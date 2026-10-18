#ifndef OPENMW_PROCESSOROBJECTSCALE_HPP
#define OPENMW_PROCESSOROBJECTSCALE_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectScale final: public BaseObjectProcessor
    {
    public:
        ProcessorObjectScale()
        {
            BPP_INIT(ID_OBJECT_SCALE)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            objectList.scaleObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTSCALE_HPP
