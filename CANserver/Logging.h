#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <Arduino.h>
#include "esp32_can/esp32_can.h"
#include <Preferences.h>
#include <map>

#include "SDCard.h"

namespace CANServer 
{
    class Logging 
    {
    public:
        typedef enum {
            LogType_Unknown = 0,
            LogType_Raw = 1,
            LogType_Interval = 2,
            LogType_Drive = 3,
            LogType_Charge = 4,
            LogType_Serial = 50,
        } LogType;

        static Logging* instance();
        ~Logging();

        void setup();

        void handleMessage(CAN_FRAME *frame, const uint8_t busId);

        const bool isActive(const LogType logtype) const;
        const size_t fileSize(const LogType logtype) const;
        const String path(const LogType logtype) const;

        void enable(const LogType logtype);
        void disable(const LogType logtype);

        void deleteFile(const LogType logtype);

        void saveConfiguraiton();

    private:
        Logging();

        static Logging *_instance;
        Preferences _prefs;

        typedef struct
        {
            String prefName;
            bool enabled;
            String path;
            SDFile fileHandle;
            uint16_t usageCounter;
        } LogDetails_t;

        typedef std::map<const LogType, LogDetails_t> LogMap;
        typedef std::pair<const LogType, LogDetails_t> LogPair;
        LogMap _logs;

        typedef std::map<const uint32_t, time_t> LogIntervalTrackingMap;
        typedef std::pair<const uint32_t, time_t> LogIntervalTrackingPair;
        typedef std::map<const uint8_t, LogIntervalTrackingMap> LogIntervalBusTrackignMap;
        typedef std::pair<const uint8_t, LogIntervalTrackingMap> LogIntervalBusTrackignPair;

        LogIntervalBusTrackignMap _intervalTracker;

        std::stringstream* _generateCandumpOutputString(std::stringstream* stringStreamToUse, const time_t tv_sec, const suseconds_t tv_usec, const CAN_FRAME* frame, const uint8_t busId);
    };
}
#endif
