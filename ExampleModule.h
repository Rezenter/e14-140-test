#ifndef LABBOT_EXAMPLEMODULE_H
#define LABBOT_EXAMPLEMODULE_H

#include <labbot/kernel/drivers/DeviceDriver.h>
#include <jsoncpp/json/json.h>

#include <dlfcn.h>
#include <bitset> 
#include <vector>
#include <sstream>

#include "include/stubs.h"
#include "include/ioctl.h"
#include "include/ifc_ldev.h"


class ExampleModule/* : public LabBot::DeviceDriver*/{
public:

    ExampleModule() = delete;
    ExampleModule (const std::string & name, const std::string & type);
    ~ExampleModule ();
    
    bool loadConfig (Json::Value & config);
    bool initModule ();
    
    bool destroyModule ();
    bool rebootModule ();
    
    bool handleRequest (LabBot::Request & req, LabBot::Response & resp);
    
    LabBot::Module::State getState ();
    bool handleFailFunction(LabBot::Request &req, LabBot::Response &resp);
    bool handleGetData(LabBot::Request &req, LabBot::Response &resp);
    bool handleSetData(LabBot::Request &req, LabBot::Response &resp);
    
    void cyclicFunc ();
    
protected:
private:
};

#endif //LABBOT_EXAMPLEMODULE_H