#ifndef OPENMW_BASEOBJECTPROCESSOR_HPP
#define OPENMW_BASEOBJECTPROCESSOR_HPP

#include "../ObjectProcessor.hpp"
#include "apps/openmw/mwmp/Main.hpp"
#include "apps/openmw/mwmp/CellController.hpp"
#include "apps/openmw/mwworld/cellstore.hpp"

namespace mwmp
{
    class BaseObjectProcessor : public ObjectProcessor
    {
    public:
        virtual void Do(ObjectPacket &packet, ObjectList &objectList)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_VERBOSE, "Received %s about %s", strPacketID.c_str(), objectList.cell.getShortDescription().c_str());
        }
    protected:
        MWWorld::CellStore *ptrCellStore;

    };
}

#endif //OPENMW_BASEOBJECTPROCESSOR_HPP
