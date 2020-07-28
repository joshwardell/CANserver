#include "Logging.h"

#include <sys/time.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

#define RAWCANLOGNAME "/CAN.raw.log"

#define FLUSH_THRESHOLD 200

CANServer::Logging* CANServer::Logging::_instance = NULL;

CANServer::Logging::Logging()
{
}


CANServer::Logging::~Logging()
{

}

CANServer::Logging* CANServer::Logging::instance()
{
    if (_instance == NULL)
    {
        _instance = new CANServer::Logging();
    }

    return _instance;
}

void CANServer::Logging::setup()
{
    _prefs.begin("Logging");

    {
        //Raw logging
        LogDetails_t logDetails;
        logDetails.prefName = String("lograw");
        logDetails.enabled = _prefs.getBool(logDetails.prefName.c_str(), false);
        logDetails.path = String(RAWCANLOGNAME);
        logDetails.usageCounter = 0;
        
        _logs.insert(CANServer::Logging::LogPair(CANServer::Logging::LogType_Raw, logDetails));

        if (logDetails.enabled)
        {
            //This will ensure that the file is open and ready to use when enabled
            this->enable(CANServer::Logging::LogType_Raw);
        }
    }
}

void CANServer::Logging::saveConfiguraiton()
{
    for (LogMap::const_iterator it = _logs.begin(); it != _logs.end(); it++)
    {
        _prefs.putBool(it->second.prefName.c_str(), it->second.enabled);
    }
}

void CANServer::Logging::enable(const CANServer::Logging::LogType logtype)
{
    LogMap::iterator it = _logs.find(logtype);
    if (it != _logs.end())
    {
        it->second.enabled = true;

        //Lets make sure the file is open (if we can open it)
        it->second.fileHandle = SD.open(it->second.path, FILE_APPEND);
        if (it->second.fileHandle)
        {
            //File opened fine.  Its ready to use
        }
        else
        {
            //There was a problem opening the file...  This should be handled some how.
        }
    }
}
void CANServer::Logging::disable(const CANServer::Logging::LogType logtype)
{
    LogMap::iterator it = _logs.find(logtype);
    if (it != _logs.end())
    {
        it->second.enabled = false;

        //if we have an open file handle close it.
        if (it->second.fileHandle)
        {
            it->second.fileHandle.close();
        }
    }
}

void CANServer::Logging::deleteFile(const CANServer::Logging::LogType logtype)
{
    //Ensure that we aren't logging to this file right now
    this->disable(logtype);

    //Remote the file from disk
    SD.remove(this->path(logtype)); 

    //If this log file is supposed to be active make sure to start it up now that we have removed the file
    if (this->isActive(logtype))
    {
        this->enable(logtype);
    }
}

const bool CANServer::Logging::isActive(const CANServer::Logging::LogType logtype) const
{
    //Get the enabled flag for this log
    LogMap::const_iterator it = _logs.find(logtype);
    if (it != _logs.end())
    {
        return it->second.enabled;
    }
    return false;
}
const size_t CANServer::Logging::fileSize(const CANServer::Logging::LogType logtype) const
{
    //Get the file size for this log
    LogMap::const_iterator it = _logs.find(logtype);
    if (it != _logs.end())
    {
        if (SD.exists(it->second.path))
        {
            SDFile rawlog = SD.open(it->second.path, FILE_READ);
            if (rawlog)
            {
                size_t fileSize = rawlog.size();
                rawlog.close();

                return fileSize;
            }
        }
    }
    return 0;
}
const String CANServer::Logging::path(const CANServer::Logging::LogType logtype) const
{
    LogMap::const_iterator it = _logs.find(logtype);
    if (it != _logs.end())
    {
        return it->second.path;
    }
    
    return "";
}

struct timeval _currentTimeOfDay = {0,0};

void CANServer::Logging::handleMessage(CAN_FRAME *frame, const uint8_t busId)
{
    gettimeofday(&_currentTimeOfDay, NULL);

    //Loop through all the loggers and let the active (and valid file handle) ones do somthing
    for (LogMap::iterator it = _logs.begin(); it != _logs.end(); it++)
    {
        if (it->second.enabled && it->second.fileHandle)
        {
            switch(it->first)
            {
                case CANServer::Logging::LogType_Raw:
                {
                    std::ostringstream ss;
                    ss << "(" 
                        << std::setw(10) << std::setfill('0') << _currentTimeOfDay.tv_sec << "." << std::setw(6) << std::setfill('0') << int(_currentTimeOfDay.tv_usec) << std::setw(0) << std::setfill(' ')
                        << ") can" << std::dec << (int)busId << " "
                        << std::setw(3) << std::setfill('0') << std::uppercase << std::hex << (uint)(frame->id) << std::setw(0) << std::setfill(' ')
                        << "#";
                    
                    for (int i = 0; i < frame->length; i++) 
                    {
                        ss << std::setw(2) << std::setfill('0') << std::uppercase << std::hex << (uint)(frame->data.byte[i]);
                    }
                    it->second.fileHandle.print(ss.str().c_str());
                    it->second.fileHandle.println();

                    //Only flush every once and a while to help keep performance somewhat sane
                    if (it->second.usageCounter++ > FLUSH_THRESHOLD)
                    {
                        it->second.usageCounter = 0;
                        it->second.fileHandle.flush();
                    }
                    break;
                }

                case CANServer::Logging::LogType_Serial:
                {
                    //Simple Serial Logging (for testing, not recomended to enable for any kinda long term)
                    break;
                } 

                default:
                {
                    //Unknown logger.  Ignore this for now.
                    break;
                }
            }
        }
    }
}