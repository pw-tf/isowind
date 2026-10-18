#ifndef OPENMW_PROCESSOROBJECTMISCELLANEOUS_HPP
#define OPENMW_PROCESSOROBJECTMISCELLANEOUS_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectMiscellaneous final: public BaseObjectProcessor
    {
    public:
        ProcessorObjectMiscellaneous()
        {
            BPP_INIT(ID_OBJECT_MISCELLANEOUS)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            objectList.setGoldPoolsForObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTMISCELLANEOUS_HPP
