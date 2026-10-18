#ifndef OPENMW_PROCESSOROBJECTANIMPLAY_HPP
#define OPENMW_PROCESSOROBJECTANIMPLAY_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectAnimPlay final: public BaseObjectProcessor
    {
    public:
        ProcessorObjectAnimPlay()
        {
            BPP_INIT(ID_OBJECT_ANIM_PLAY)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            objectList.animateObjects(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTANIMPLAY_HPP
