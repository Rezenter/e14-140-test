
#include "ExampleModule.h"

#include <iostream> //debug purpose only

typedef IDaqLDevice* (*CREATEFUNCPTR)(ULONG Slot);
CREATEFUNCPTR CreateInstance;
IDaqLDevice* pI;
std::bitset<16> ttlState; //saving RAM)


using namespace std;

FLUG_DYNAMIC_DRIVER(ExampleModule); // Класс регистрируется в качестве модуля, пригодного для использования LabBot

ExampleModule::ExampleModule(const std::string &deviceInstance, const std::string &deviceType) : DeviceDriver(
deviceInstance, deviceType){
    void* handle;
    char* error;

    handle = dlopen("liblcomp.so",RTLD_LAZY);
    if(!handle){
        throw std::runtime_error("error opening shared object library file"); //more information in dlerror()
    }
    CreateInstance =(CREATEFUNCPTR) dlsym(handle,"CreateInstance");
    if((error = dlerror())!=NULL)
    {
        throw std::runtime_error(error);
    }
    ttlState.reset(); //default ttl outputs state is low
}

ExampleModule::~ExampleModule(){
    destroyModule();
}

bool ExampleModule::loadConfig(Json::Value &config) { // Загружаем конфигурацию устройства.
// Не будет вызвана,если отсутствует структура config у данного устройства в конфигурационном файле
    return true; //unused
}

bool ExampleModule::initModule() { // Подключаемся к устройству, инициализируем соединения, итп
    LUnknown* pIUnknown = CreateInstance(0); //for single board only
    switch(errno){ 
        case 0:
            //ok
            break;
        case 3:
            throw std::runtime_error("kernel module for usblcomp is not loaded");
            break; /*programm should terminate at this state, and i wont need to break,
                    but compiller may rise an exeption without it*/
        case 19:
            throw std::runtime_error("usb device is not connected");//maybe i should just return false
            break;
        default:
            throw std::runtime_error("unknown initialisation error"); //other error codes may be found in documentation if needed
            break;
    }
    if(pIUnknown == NULL) { //may be overkill: errno should detect all errors
        throw std::runtime_error("CreateInstance error");
    }
    
    HRESULT hr = pIUnknown->QueryInterface(IID_ILDEV,(void**)&pI);
    if(hr!=S_OK) {
        throw std::runtime_error("Get IDaqLDevice failed");
    }
    pIUnknown->Release();
    HANDLE hVxd = pI->OpenLDevice();

    SLOT_PAR sl;
    pI->GetSlotParam(&sl);
    /*
    if(pI->PlataTest() != L_SUCCESS){ //for e14-140 always returns true
        cout  << "Plata Test failed\n";
        return false;
    }
    */
    PLATA_DESCR_U2 pd;
    pI->ReadPlataDescr(&pd);
    if(sl.BoardType != E140){ //this module is for e140 only
        return false;
    }
    /* //some usefull data, which may be used later
    cout  << "SerNum       " << pd.t5.SerNum << endl;
    cout  << "BrdName      " << pd.t5.BrdName << endl;
    cout  << "Rev          " << pd.t5.Rev << endl;
    cout  << "Quartz       " << dec << pd.t5.Quartz << endl;
    */
    return true;
}

bool ExampleModule::destroyModule() { // Освобождаем ресурсы, закрываем дескрипторы, итп.
    if(pI == NULL){
        return true; //not created => already destroyed
    }
    /* //setOutputs to zero
    Json::Value req;
    for (int i = 0; i < 16; ++i) {
        req["terminals"][i] = i;
        req["values"][i] = 0;
    }
    req["reqtype"] = "setAnalogOutputs";
    req["subsystem"] = "iDontKnow";
    LabBot::Response unused;
    //req to LabBot::Request convertion needed
    handleSetData(LabBot::Request , unused); //set DAC to zero
    req["reqtype"] = "setDigitalOutputs"; 
    //req to LabBot::Request convertion needed
    handleSetData(LabBot::Request , unused); //set TTL to zero
    */
    
    pI->CloseLDevice(); //may be this step can stuck. check later
    pI->Release(); //same
    return true;
}

bool ExampleModule::rebootModule() { // Функция, выполняемая при перезагрузке модуля устройства.
// Приведена реализация по умолчанию, но может быть реализована произвольная логика
    return destroyModule() && initModule(); //all errors are handelled in destroy/initialise functions
}

// Основная функция, вызваемая для обработки запросов
bool ExampleModule::handleRequest(LabBot::Request &req, LabBot::Response &resp){
    if(pI == NULL && getState() == LabBot::Module::State::ST_ONLINE){
        return false; //nothing to do, if device isn`t properly initialised
    }
    if (!req.m_json.isMember("subsystem")) {
        throw std::runtime_error("subsystem is not specified"); //no need to return something. it is an emergency termination
    }  
    if (!req.m_json.isMember("reqtype")) {
        throw std::runtime_error("reqtype is not specified");
    }
    std::string reqtype = req.m_json["reqtype"].asString();
    if (reqtype == "getAnalogInputs" || reqtype == "getDigitalInputs") {
        return handleGetData(req, resp);
    }
    if (reqtype == "setAnalogOutputs" || reqtype == "setDigitalOutputs") {
        return handleSetData(req, resp);
    }
    if (reqtype == "fail") {
        return handleFailFunction(req, resp);
    }
    return false;
}

LabBot::Module::State ExampleModule::getState() { // Используется менеджером устройств
    if(pI == NULL || pI->PlataTest() != L_SUCCESS){ //for e14-140 PlataTest() always returns true
        return LabBot::Module::State::ST_OFFLINE;
    }
    return LabBot::Module::State::ST_ONLINE;
}


bool ExampleModule::handleGetData(LabBot::Request &req, LabBot::Response &resp){
    Json::Value ret; // В этом объекте будет формироваться ответ на запрос
    std::string reqType = req.m_json["reqtype"].asString();
    ASYNC_PAR asyncPar;
    vector<float> data;
    if(reqType == "getAnalogInputs"){
        asyncPar.s_Type = L_ASYNC_ADC_INP;
        if (!req.m_json.isMember("terminals")) {
            //this kind of exeptions was in example, i`d rather return false instead of terminating everything
            throw std::runtime_error("terminals are not specified"); //no need to return something. it is an emergency termination
        }
        vector<int> terminals;
        string s;    
        istringstream inp(req.m_json["terminals"].asString());
        while(std::getline(inp, s, ',')){
            int t = stoi(s);
            if(t < 0 || t > 15){ //there are 16 differential analog inputs
                throw std::runtime_error("terminals are out of bound");
            }
            terminals.push_back(t); //reading same terminal several times is legal
        }            
        for(int i = 0; i < terminals.size(); ++i){ /*as far as i got it, lcomp for linux allows reading/writing only from
            the first element of asyncPar.Data. So i have to access analog pins one by one.*/
            asyncPar.NCh = 1;
            asyncPar.Chn[0] = 0; //clear 
            for(int shift = 0; shift < 4; ++shift){ 
                asyncPar.Chn[0] += ((terminals.at(i) >> shift)&1) << shift; //ch number, replace i
            }
            asyncPar.Chn[0] += 0 << 4; //zero config
            asyncPar.Chn[0] += 0 << 5; //diff mode
            asyncPar.Chn[0] += 0 << 6; //gain 0 //default gain = 1               
            asyncPar.Chn[0] += 0 << 7; //gain 1
            pI->IoAsync(&asyncPar);
            data.push_back(asyncPar.Data[0]);
        } 
    }else{
        if (!req.m_json.isMember("terminals")) {
            throw std::runtime_error("terminals are not specified"); //no need to return something. it is an emergency termination
        }
        vector<int> terminals;
        string s;    
        istringstream inp(req.m_json["terminals"].asString());
        while(std::getline(inp, s, ',')){
            int t = stoi(s);
            if(t < 0 || t > 15){
                throw std::runtime_error("terminals are out of bound");
            }
            terminals.push_back(t); //reading same terminal several times is legal, but useless in this particular realisation
        }            
        asyncPar.s_Type = L_ASYNC_TTL_INP;
        asyncPar.NCh = 16;
        pI->IoAsync(&asyncPar);
        for(int i = 0; i < 16; ++i){
            if(find(terminals.begin(), terminals.end(), i) != terminals.end()){//send data from only required terminals 
                data.push_back((asyncPar.Data[0] >> i)&1);
            }
        }
    }
    for (int i = 0; i < data.size(); ++i) {
        ret["data"][i] = data.at(i);
    }
    ret["status"] = "success";
    resp = ret;
    return true;
}

bool ExampleModule::handleSetData(LabBot::Request &req, LabBot::Response &resp){
    Json::Value ret; // В этом объекте будет формироваться ответ на запрос
    std::string reqType = req.m_json["reqtype"].asString();
    ASYNC_PAR asyncPar;
    vector<float> data;
    if(reqType == "setAnalogOutputs"){ //rise [0, 2048), fall [2048, 4096); 
        asyncPar.s_Type = L_ASYNC_DAC_OUT;
        if (!req.m_json.isMember("terminals")) {
            //this kind of exeptions was in example, i`d rather return false instead of terminating everything
            throw std::runtime_error("terminals are not specified"); //no need to return something. it is an emergency termination
        }
        vector<int> terminals;
        string s;    
        istringstream inp(req.m_json["terminals"].asString());
        while(std::getline(inp, s, ',')){
            int t = stoi(s);
            if(t != 0 && t != 1){
                throw std::runtime_error("terminals are out of bound");
            }
            terminals.push_back(t); //reading same terminal several times is legal
        }
        vector<int> values;
        inp = istringstream(req.m_json["values"].asString());
        while(std::getline(inp, s, ',')){
            int t = stoi(s);
            if(t < 0 || t > 4095){
                throw std::runtime_error("values are out of bound");
            }
            values.push_back(t);
        }           
        if(values.size() != terminals.size()){
            throw std::runtime_error("values dont corespond terminals");
        } 
        for(int i = 0; i < terminals.size(); ++i){
            asyncPar.Mode = i;
            asyncPar.Data[0] = values[i];
            pI->IoAsync(&asyncPar);
            
        }
    }else{
        asyncPar.NCh = 16;
        asyncPar.s_Type = L_ASYNC_TTL_CFG;
        asyncPar.Mode = 1;  //enable ttl
        pI->IoAsync(&asyncPar);

        if (!req.m_json.isMember("terminals")) {
            //this kind of exeptions was in example, i`d rather return false instead of terminating everything
            throw std::runtime_error("terminals are not specified"); //no need to return something. it is an emergency termination
        }
        //i like copy-paste code)
        vector<int> terminals;
        string s;    
        istringstream inp(req.m_json["terminals"].asString());
        while(std::getline(inp, s, ',')){
            int t = stoi(s);
            if(t < 0 || t > 15){
                throw std::runtime_error("terminals are out of bound");
            }
            terminals.push_back(t); //writing to the same terminal several times is legal, but kinda useless
        }
        vector<int> values;
        inp = istringstream(req.m_json["values"].asString());
        while(std::getline(inp, s, ',')){
            int t = stoi(s);
            if(t != 0 || t != 1){
                throw std::runtime_error("values are out of bound");
            }
            values.push_back(t);
        }           
        if(values.size() != terminals.size()){
            throw std::runtime_error("values dont corespond terminals");
        } 
        asyncPar.NCh = 16;
        asyncPar.s_Type = L_ASYNC_TTL_OUT;
        //all not mensioned terminals are leaved in their previous state.
        for(int i = 0; i < terminals.size(); ++i){//i dont think there are any reasons to use iterators here
            if(terminals.at(i) == 0){
                ttlState.reset(i);
            }else{
                ttlState.set(i);
            }
        }
        asyncPar.Data[0] = 0;
        for(int shift = 0; shift < 16; ++shift){ 
            asyncPar.Data[0] += (ttlState[shift] << shift);
        }
        pI->IoAsync(&asyncPar);
    }
    ret["status"] = "success";
    resp = ret;
    return true;
}

bool ExampleModule::handleFailFunction(LabBot::Request &req, LabBot::Response &resp) {
    throw std::runtime_error("handleFailFunction always fails"); // Строка с описанием ошибки будет аккуратно упакована
                                                                // и отправлена на клиент в корректной JSON-структуре
                                                                //no need to return something. it is an emergency termination
}