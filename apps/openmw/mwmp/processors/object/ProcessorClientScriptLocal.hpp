#ifndef OPENMW_PROCESSORCLIENTSCRIPTLOCAL_HPP
#define OPENMW_PROCESSORCLIENTSCRIPTLOCAL_HPP

#include "BaseObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorClientScriptLocal final: public BaseObjectProcessor
    {
    public:
        ProcessorClientScriptLocal()
        {
            BPP_INIT(ID_CLIENT_SCRIPT_LOCAL)
        }

        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            BaseObjectProcessor::Do(packet, objectList);

            ptrCellStore = Main::get().getCellController()->getCellStore(objectList.cell);

            if (!ptrCellStore) return;

            objectList.setClientLocals(ptrCellStore);
        }
    };
}

#endif //OPENMW_PROCESSORCLIENTSCRIPTLOCAL_HPP
