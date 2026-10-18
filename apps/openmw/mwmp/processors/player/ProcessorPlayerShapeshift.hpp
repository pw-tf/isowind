#ifndef OPENMW_PROCESSORPLAYERSHAPESHIFT_HPP
#define OPENMW_PROCESSORPLAYERSHAPESHIFT_HPP

#include "../PlayerProcessor.hpp"

namespace mwmp
{
    class ProcessorPlayerShapeshift final: public PlayerProcessor
    {
    public:
        ProcessorPlayerShapeshift()
        {
            BPP_INIT(ID_PLAYER_SHAPESHIFT)
        }

        virtual void Do(PlayerPacket &packet, BasePlayer *player)
        {
            LOG_MESSAGE_SIMPLE(TimedLog::LOG_INFO, "Received ID_PLAYER_SHAPESHIFT from server");

            if (isLocal())
            {
                LOG_APPEND(TimedLog::LOG_INFO, "- Packet was about LocalPlayer");

                static_cast<LocalPlayer *>(player)->setShapeshift();
            }
            else if (player != 0)
            {
                static_cast<DedicatedPlayer *>(player)->setShapeshift();
            }
        }
    };
}

#endif //OPENMW_PROCESSORPLAYERSHAPESHIFT_HPP
