#ifndef OPENMW_PROCESSOROBJECTMOVE_HPP
#define OPENMW_PROCESSOROBJECTMOVE_HPP

#include "../ObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectMove : public ObjectProcessor
    {
    public:
        ProcessorObjectMove()
        {
            BPP_INIT(ID_OBJECT_MOVE)
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTMOVE_HPP
