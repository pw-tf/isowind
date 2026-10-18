#ifndef OPENMW_PROCESSOROBJECTACTIVATE_HPP
#define OPENMW_PROCESSOROBJECTACTIVATE_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectActivate final: public BaseObjectProcessor
    {
    public:
        ProcessorObjectActivate()
        {
            BPP_INIT(ID_OBJECT_ACTIVATE)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            objectList.activateObjects(ptrCellStore);
        }

    };
}

#endif //OPENMW_PROCESSOROBJECTACTIVATE_HPP
