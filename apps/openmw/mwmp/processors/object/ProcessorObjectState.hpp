#ifndef OPENMW_PROCESSOROBJECTSTATE_HPP
#define OPENMW_PROCESSOROBJECTSTATE_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectState final: public BaseObjectProcessor
    {
    public:
        ProcessorObjectState()
        {
            BPP_INIT(ID_OBJECT_STATE)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            objectList.setObjectStates(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTSTATE_HPP
