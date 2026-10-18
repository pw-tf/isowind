#ifndef OPENMW_PROCESSOROBJECTANIMPLAY_HPP
#define OPENMW_PROCESSOROBJECTANIMPLAY_HPP

#include "../ObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorObjectAnimPlay : public ObjectProcessor
    {
    public:
        ProcessorObjectAnimPlay()
        {
            BPP_INIT(ID_OBJECT_ANIM_PLAY)
        }
    };
}

#endif //OPENMW_PROCESSOROBJECTANIMPLAY_HPP
