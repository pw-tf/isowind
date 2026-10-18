#ifndef OPENMW_PROCESSOROBJECTSPAWN_HPP
#define OPENMW_PROCESSOROBJECTSPAWN_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectSpawn final: public BaseObjectProcessor
    {
    public:
        ProcessorObjectSpawn()
        {
            BPP_INIT(ID_OBJECT_SPAWN)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            objectList.spawnObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTSPAWN_HPP
