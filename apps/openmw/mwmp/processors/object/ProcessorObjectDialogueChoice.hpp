#ifndef OPENMW_PROCESSOROBJECTDIALOGUECHOICE_HPP
#define OPENMW_PROCESSOROBJECTDIALOGUECHOICE_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectDialogueChoice final: public BaseObjectProcessor
    {
    public:
        ProcessorObjectDialogueChoice()
        {
            BPP_INIT(ID_OBJECT_DIALOGUE_CHOICE)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            objectList.makeDialogueChoices(ptrCellStore);
        }

    };
}

#endif //OPENMW_PROCESSOROBJECTDIALOGUECHOICE_HPP
