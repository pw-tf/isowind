#ifndef OPENMW_PROCESSOROBJECTDELETE_HPP
#define OPENMW_PROCESSOROBJECTDELETE_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectDelete final: public BaseObjectProcessor
    {
    public:
        ProcessorObjectDelete()
        {
            BPP_INIT(ID_OBJECT_DELETE)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            objectList.deleteObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTDELETE_HPP
