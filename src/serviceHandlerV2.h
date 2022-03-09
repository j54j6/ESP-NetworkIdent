#pragma once
#ifndef NODEWORK_SERVICEHANDLER
#define NODEWORK_SERVICEHANDLER

//include Stuff
//public Libs
#include <Arduino.h> //Basic Lib
#include <ESP8266WiFi.h> //WifiControl
#include "structs.h"
#include <WiFiUdp.h> //base for UDP Manager
#include <ArduinoJson.h> //useage of JSON 

//own Libs
#include "filemanager.h" //Filemanager based on LittleFS with Config Addons 
#include "logger.h" //Serial and File Logging
#include "wifiManagerV2.h" //control of Wifi Interface
#include "udpManager.h" //udp Manager to control and receive UDP connection/packets
#include "../../lib/linkedList/src/linkedList.h"

#define networkIdentPort 63549
#define networkSearchRequestTimeout 5000 //after x ms the request will stopped
#define blockedTime 60000 //60s
#define activeTimeout 10000 // 10s
#define preferExternalService //preferInternalService - On Request - if internal and external exist use defined
#define servicesPath "services/" // self.json -> For self offered Services AND <<serviceName>>.json for external services

class ServiceHandlerV2 {
    private:
        SysLogger* logger;
        WiFiManager* _WiFi;
        Filemanager* _FM;
        udpManager* _UDPM;

        boolean networkIdentActive = false;
        LinkedList<searchRequest> activeSearches;
        LinkedList<searchRequest> blockedSearches;

        boolean addNewInternalService(searchRequest *response);
        boolean addNewExternalService(searchRequest *response);

        boolean isSerivceInActiveSearches(String serviceName);
        boolean isServiceBlocked(String serviceName);

        searchRequest* getBlockedSearchRequest(String serviceName);
        searchRequest* getActiveSearchRequest(String serviceName);

        void removeBlockedSearchRequest(searchRequest* request);
        void removeActiveSearchRequest(searchRequest* request);

        //Get Service helper
        boolean checkForSelfOfferedService(Service &serviceData); //return ServiceData and true, if found else return false
        boolean checkForExternalService(Service &serviceData);

        void checkForRequests();
        boolean validatePacket(DynamicJsonDocument *packet);

        void handleRequest(DynamicJsonDocument *packet);
        void handleResponse(DynamicJsonDocument *packet);

        boolean sendSearchResponse(Service &serviceData, DynamicJsonDocument *packet);

        void handleBlockedQueue();
        void handleActiveQueue();
    protected:
        
    public:
        ServiceHandlerV2(WiFiManager* _WiFi, Filemanager* _FM)
        {
            #ifdef compileLoggingClassCritical
                logger = new SysLogger("ServiceHandler-V2");
            #endif
            this->_WiFi = _WiFi;
            this->_FM = _FM;
            this->_UDPM = new udpManager(this->_FM, this->_WiFi, networkIdentPort);
        }

        Service getService(String serviceName, boolean onlyExternal = false, boolean preferExternal = true);
        boolean checkForService(String serviceName);
        void removeService(String serviceName, boolean internal = false);
        short getIsServicelocked(String serviceName);
        searchRequest* startSearchForService(String serviceName);
        void stopSearch(String serviceName);
        
        boolean addNewInternalServiceMan(String serviceName, int port);
        boolean addNewExternalServiceMan(String serviceName, IPAddress ip, int port, String MAC = "n.A");

        boolean changeExistingService(String serviceName, IPAddress ip, int port, String mac = "n.A");
        boolean addService(String serviceName, IPAddress ip, int port, String mac = "n.A");
        searchRequest* sendSearchRequest(String neededServiceName);

        void begin(); //start listening to requests
        void stop(); //stop listening to requests
        void run();
};
#endif