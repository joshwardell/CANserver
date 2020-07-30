#ifndef __CANBUS_H__
#define __CANBUS_H__

#include <esp32_can.h>
#include <map>

namespace CANServer 
{
    class CanBus 
    {
    public:

        class AnalysisItem
        {
        public:
            AnalysisItem();
            
            uint32_t _frameId;
            uint8_t _startBit;
            uint8_t _bitLength;
            double _factor;
            int _signalOffset;
            bool _isSigned;
            bool _byteOrder;

            float _lastValue;
        };

        typedef std::map<std::string, AnalysisItem*> AnalysisItemMap;
        typedef std::pair<std::string, AnalysisItem*> AnalysisItemPair;
       
        static CanBus* instance();
        ~CanBus();

        void setup();
        void startup();

        void handle();

        AnalysisItemMap* dynamicAnalysisItems() { return &_analysisItems; };

        void pauseDynamicAnalysis();
        void resumeDynamicAnalysis();

        void saveDynamicAnalysisConfiguration();

    private:
        CanBus();

        static CanBus *_instance;

        CAN_FRAME _workingCANFrame;

        void _processFrame(CAN_FRAME *frame, const uint8_t busId);
        void _processStaticAnalysis(CAN_FRAME *frame);
        void _processDynamicAnalysis(CAN_FRAME *frame);

        void _loadDynamicAnalysisConfiguration();

        bool _dynamicAnalysisPaused;
        AnalysisItemMap _analysisItems;
    };
}
#endif