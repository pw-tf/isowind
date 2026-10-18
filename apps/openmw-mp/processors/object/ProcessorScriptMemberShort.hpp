#ifndef OPENMW_PROCESSORSCRIPTMEMBERSHORT_HPP
#define OPENMW_PROCESSORSCRIPTMEMBERSHORT_HPP

#include "../ObjectProcessor.hpp"

namespace mwmp
{
    class ProcessorScriptMemberShort : public ObjectProcessor
    {
    public:
        ProcessorScriptMemberShort()
        {
            BPP_INIT(ID_SCRIPT_MEMBER_SHORT)
        }
    };
}

#endif //OPENMW_PROCESSORSCRIPTMEMBERSHORT_HPP
