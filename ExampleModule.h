#ifndef LABBOT_EXAMPLEMODULE_H
#define LABBOT_EXAMPLEMODULE_H

#include <labbot/kernel/drivers/DeviceDriver.h>
#include <jsoncpp/json/json.h>

#include <dlfcn.h>
#include <bitset> 
#include <vector>

#define LCOMP_LINUX 1
#define INITGUID
#include "include/stubs.h"
#include "include/ioctl.h"
#include "include/ifc_ldev.h"

typedef IDaqLDevice* (*CREATEFUNCPTR)(ULONG Slot);

class ExampleModule : public LabBot::DeviceDriver{
public:
    ExampleModule() = delete;
    ~ExampleModule() override ;
    ExampleModule (const std::string & deviceInstance, const std::string & deviceType);

    bool loadConfig (Json::Value & config) override;
    bool initModule () override;
    bool destroyModule () override;
    bool handleRequest (LabBot::Request & req, LabBot::Response & resp) override;
    bool rebootModule () override;
    LabBot::Module::State getState() override;

    
protected:
    bool handleFailFunction(LabBot::Request &req, LabBot::Response &resp);
    bool handleGetData(LabBot::Request &req, LabBot::Response &resp);
    bool handleSetData(LabBot::Request &req, LabBot::Response &resp);

private:
    void* handle;
    CREATEFUNCPTR CreateInstance;
    IDaqLDevice* pI;
    std::bitset<16> ttlState; //saving RAM)
    char* error;
};

#endif //LABBOT_EXAMPLEMODULE_H