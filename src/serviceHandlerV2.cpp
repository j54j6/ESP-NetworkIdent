#include "serviceHandlerV2.h"

boolean ServiceHandlerV2::checkForService(String serviceName)
{
    Service requestedService;
    requestedService.serviceName = serviceName;

    boolean internalExist = checkForSelfOfferedService(requestedService);
    boolean externalExist = checkForExternalService(requestedService);

    if(internalExist || externalExist)
    {
        return true;
    }
    return false;
}

boolean ServiceHandlerV2::checkForSelfOfferedService(Service &serviceData)
{
    String fileName = String(servicesPath) + "self.json";
    logDebug("checkForSelfOfferedService", "Check for offered Service " + String(serviceData.serviceName.c_str()));
    if(this->_FM->fExist(fileName.c_str()))
    {
        logDebug("checkForSelfOfferedService", "Found offered Services file - check for Service");
        //File exist check for key with this serviceName then give prot number
        if(this->_FM->checkForKeyInJSONFile(fileName.c_str(), serviceData.serviceName.c_str()))
        {
            String port = this->_FM->readJsonFileValue(fileName.c_str(), serviceData.serviceName.c_str());
            logDebug("checkForSelfOfferedService", "Service " + String(serviceData.serviceName) + " found! - Port: " + port);
            serviceData.ip = WiFi.localIP();
            serviceData.external = false;
            serviceData.MAC = WiFi.macAddress();
            serviceData.port = port.toInt();
            return true;
        }
        else
        {
            logInfo("checkForSelfOfferedService", "Service " + String(serviceData.serviceName) + " not found!");
            return false;
        }
    }
    else
    {
        logDebug("checkForSelfOfferedService", "There are no offered Services on this device! - return false");
        return false;
    }
    return false;
}

boolean ServiceHandlerV2::checkForExternalService(Service &serviceData)
{
    logDebug("checkForExternalService", "Check for offered Service " + String(serviceData.serviceName.c_str())); 
    String fileName = servicesPath + String(serviceData.serviceName) + ".json";
    if(this->_FM->fExist(fileName.c_str()))
    {
        logDebug("checkForExternalService", "Service found!");
        if(this->_FM->checkForKeyInJSONFile(fileName.c_str(), "ip"))
        {
            String ip = this->_FM->readJsonFileValueAsString(fileName.c_str(), "ip");
            IPAddress rServiceIP;
            boolean convertIP = rServiceIP.fromString(ip);
            if(!convertIP)
            {
                logError("checkForExternalService", "Error while converting IP from file (" + ip + ") to valid IPAddress!");
                return false;
            }
            serviceData.ip = rServiceIP;
        }
        else
        {
            logError("checkForExternalService", "There is no IP Address in Service Config! cant read Config! - delete");
            removeService(serviceData.serviceName);
            return false;
        }

        if(this->_FM->checkForKeyInJSONFile(fileName.c_str(), "port"))
        {
            String port = this->_FM->readJsonFileValueAsString(fileName.c_str(), "port");
            int servicePort = port.toInt();
            serviceData.port = servicePort;
        }
        else
        {
            logError("checkForExternalService", "There is no Port in Service Config! cant read Config! - delete");
            removeService(serviceData.serviceName);
            return false;
        }

        //Service MAC is optional
        if(this->_FM->checkForKeyInJSONFile(fileName.c_str(), "mac"))
        {
            String mac = this->_FM->readJsonFileValueAsString(fileName.c_str(), "mac");
            serviceData.MAC = mac;
        }
        else
        {
            serviceData.MAC = "-1";
        }
        serviceData.external = true;
        return true;
    }
    return false;
}

Service ServiceHandlerV2::getService(String serviceName, boolean onlyExternal, boolean preferExternal)
{
    boolean internalService = false;
    boolean externalService = false;

    Service internalServiceData;
    internalServiceData.serviceName = serviceName;

    Service externalServiceData;
    externalServiceData.serviceName = serviceName;

    if(!onlyExternal)
    {
        internalService = this->checkForSelfOfferedService(internalServiceData);
    }
    externalService = this->checkForExternalService(externalServiceData);

    if(!internalService || !externalService)
    {
        if(internalService)
        {
            logDebug("getService", "Only found internal offered Service for " + serviceName)
            return internalServiceData;
        }
        else
        {
            logDebug("getService", "Only found external offered Service for " + serviceName)
            return externalServiceData;
        }
    }
    else
    {
        if(preferExternal)
        {
            logDebug("getService", "External service is prefered - return it, found both")
            return externalServiceData;
        }
        else
        {
            logDebug("getService", "Internal service is prefered - return it, found both")
            return internalServiceData;
        }
    }
    return externalServiceData;
}

void ServiceHandlerV2::removeService(String serviceName, boolean internal)
{
    if(internal)
    {
        logDebug("removeService", "Remove internal Service " + serviceName)
        Service dummy;
        dummy.serviceName = serviceName;
        boolean internalExist = checkForSelfOfferedService(dummy);

        if(internalExist)
        {
            String fileName = String(servicesPath) + "self.json";
            this->_FM->delJsonKeyFromFile(fileName.c_str(), serviceName.c_str());
            logInfo("removeService", "Service " + serviceName + " removed")
            return;
        }
        else
        {
            logWarn("removeService", "There is no internal service called " + serviceName)
            return;
        }
    }
    else
    {
        logDebug("removeService", "Remove external Service " + serviceName)
        Service dummy;
        dummy.serviceName = serviceName;
        boolean externalExist = checkForExternalService(dummy);

        if(externalExist)
        {
            String fileName = String(servicesPath) + serviceName + ".json";
            this->_FM->fRemove(fileName.c_str());
            logInfo("removeService", "Service " + serviceName + " removed")
            return;
        }
        else
        {
            logWarn("removeService", "There is no external service called " + serviceName)
            return;
        }
    }
}

boolean ServiceHandlerV2::isSerivceInActiveSearches(String serviceName)
{
    int lengthOfActiveSearchList = activeSearches.size();

    if(lengthOfActiveSearchList > 0)
    {
        boolean foundService = false;
        for(int i = 0; i < lengthOfActiveSearchList; i++)
        {
            if(activeSearches.get(i).requestedServiceName == serviceName)
            {
                foundService = true;
            }
            return foundService;
        }
    }
    else
    {
        return false;
    }
    return false;
}

boolean ServiceHandlerV2::isServiceBlocked(String serviceName)
{
    int lengthOfBlockedList = blockedSearches.size();

    if(lengthOfBlockedList > 0)
    {
        boolean foundService = false;
        for(int i = 0; i < lengthOfBlockedList; i++)
        {
            if(blockedSearches.get(i).requestedServiceName == serviceName)
            {
                foundService = true;
            }
            return foundService;
        }
    }
    else
    {
        return false;
    }
    return false;
}

void ServiceHandlerV2::removeBlockedSearchRequest(searchRequest* request)
{
    boolean blocked = isServiceBlocked(request->requestedServiceName);

    if(!blocked)
    {
        logWarn("removeBlockedSearchRequest", "Given Service request ist not blocked!")
        return;
    }

    int lengthOfBlockedList = blockedSearches.size();

    if(lengthOfBlockedList > 0)
    {
        for(int i = 0; i < lengthOfBlockedList; i++)
        {
            if(blockedSearches.get(i).requestedServiceName == request->requestedServiceName)
            {
                blockedSearches.remove(i);
                logTrace("removeBlockedSearchRequest", "Request removed!")
            }
            return;
        }
    }
    else
    {
        return;
    }
}

void ServiceHandlerV2::removeActiveSearchRequest(searchRequest* request)
{
    boolean isActive = isSerivceInActiveSearches(request->requestedServiceName);
    if(!isActive)
    {
        logWarn("removeActiveSearchRequest", "Given Service request ist not blocked!")
        return;
    }

    int lengthOfActiveSearchList = activeSearches.size();

    if(lengthOfActiveSearchList > 0)
    {
        for(int i = 0; i < lengthOfActiveSearchList; i++)
        {
            if(activeSearches.get(i).requestedServiceName == request->requestedServiceName)
            {
                this->activeSearches.remove(i);
                logTrace("removeActiveSearchRequest", "Request removed!")
            }
            return;
        }
    }
    else
    {
        return;
    }
}

searchRequest* ServiceHandlerV2::getBlockedSearchRequest(String serviceName)
{
    int lengthOfBlockedList = blockedSearches.size();

    if(lengthOfBlockedList > 0)
    {
        for(int i = 0; i < lengthOfBlockedList; i++)
        {
            if(blockedSearches.get(i).requestedServiceName == serviceName)
            {
                return blockedSearches.getPointed(i);
            }
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
    return NULL;
}

searchRequest* ServiceHandlerV2::getActiveSearchRequest(String serviceName)
{
    int lengthOfActiveSearches = activeSearches.size();

    if(lengthOfActiveSearches > 0)
    {
        for(int i = 0; i < lengthOfActiveSearches; i++)
        {
            if(activeSearches.get(i).requestedServiceName == serviceName)
            {
                return activeSearches.getPointed(i);
            }
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
    return NULL;
}

searchRequest* ServiceHandlerV2::sendSearchRequest(String neededServiceName)
{
    boolean searchActive = this->isSerivceInActiveSearches(neededServiceName);
    boolean blocked = this->isServiceBlocked(neededServiceName);

    if(searchActive)
    {
        return this->getActiveSearchRequest(neededServiceName);
    }
    else if(blocked)
    {
        return NULL;
    }

    searchRequest newRequest;


    String id = this->_WiFi->getMacAddress();
    id += String(long(random(0x00,0xFFFFFFFF)));

    logTrace("sendSearchRequest", "ID for request is: " + id);
    logDebug("sendSearchRequest", "Preparing request...");

    newRequest.searchID = id;
    newRequest.requestedServiceName = neededServiceName;

    String message = "{\"type\" : \"request\",";
    message += "\"serviceName\": \"" + neededServiceName + "\",";
    message += "\"id\": \"" + id + "\"}";
    IPAddress tarIP = WiFi.broadcastIP();
    
    Serial.println(tarIP);
    boolean requestSent = this->_UDPM->sendUdpMessage(message.c_str(), tarIP, networkIdentPort);
    newRequest.startTimestamp = millis();
    if(requestSent)
    {
        logInfo("sendSearchRequest", "Request started");
        newRequest.status = -1;
        this->activeSearches.add(newRequest);
        searchRequest *p = this->activeSearches.getPointed(activeSearches.size() -1);
        return p;
    }
    else
    {
        logError("sendSearchRequest", "Error while sending request!");
        return NULL;
    }
    return NULL;
}

searchRequest* ServiceHandlerV2::startSearchForService(String serviceName)
{
    boolean serviceBlocked = isServiceBlocked(serviceName);
    boolean serviceAlreadyInSearch = isSerivceInActiveSearches(serviceName);

    if(serviceBlocked || serviceAlreadyInSearch)
    {
        if(serviceBlocked)
        {
            return NULL; //Service is blocked 
        }
        else
        {
            return  NULL; //Service already in search -> Pending and wait for result
        }
    }
    logInfo("startSearchForService", "Start searching for Service " + serviceName);
    searchRequest* successfullyStarted = this->sendSearchRequest(serviceName);
    return successfullyStarted;
}

boolean ServiceHandlerV2::validatePacket(DynamicJsonDocument *packet)
{
    if(packet != NULL)
    {
        boolean typeKeyExist = packet->containsKey("type");
        boolean serviceNameExist = packet->containsKey("serviceName");
        boolean idExist = packet->containsKey("id");

        if(typeKeyExist and serviceNameExist && idExist)
        {
            if(packet->getMember("type") == "request")
            {
                return true;
            }
            else
            {
                //response
                boolean ipExist = packet->containsKey("ip");
                boolean portExist = packet->containsKey("port");

                if(ipExist and portExist)
                {
                    return true;
                }
                return false;
            }
        }
        return false;
    }
    return false;
}

void ServiceHandlerV2::checkForRequests()
{
    udpPacketResolve* packet = this->_UDPM->getLastUDPPacketLoop();

    if(packet->udpContent != "NULL")
    {
        logDebug("checkForRequest", "Packet received")
        //Parse content in JSON
        //If there is any new Data -> Content != NULL -> clean all fragments delivered with the protocol -> in this Case only char until packackgeSize[n] will be readed all other are removed
        if(packet->udpContent != "NULL")
        {
            packet->clean(); //delete fragments at the end of document
        }

        DynamicJsonDocument receivedPacketAsJSON(425);
        DeserializationError error = deserializeJson(receivedPacketAsJSON, packet->udpContent);
        receivedPacketAsJSON["srcIP"] = packet->remoteIP.toString();
        receivedPacketAsJSON["srcPort"] = String(packet->remotePort);
        receivedPacketAsJSON["udpPacketSize"] = String(packet->paketSize);

        if(error)
        {
            logError("checkForRequest", "Error while deserialize udp Content! - Error: " + String(error.c_str()));
            packet->resetPack();
            return; 
        }
        logDebug("checkForRequest", "Packet successfully converted!")
        boolean packetIsValid = this->validatePacket(&receivedPacketAsJSON);

        if(packetIsValid)
        {
            logDebug("checkForRequest", "Packet validated!")
            if(receivedPacketAsJSON["type"] == "request")
            {
                logDebug("checkForRequest", "Incoming request handler called")
                handleRequest(&receivedPacketAsJSON);
            }
            else if(receivedPacketAsJSON["type"] == "response")
            {
                logDebug("checkForRequest", "Incoming response handler called")
                handleResponse(&receivedPacketAsJSON);
            }
            packet->resetPack();
        }
    }
}

boolean ServiceHandlerV2::sendSearchResponse(Service &serviceData, DynamicJsonDocument *packet)
{
    if(packet->getMember("ip") == "0.0.0.0")
    {
        logError("sendSearchResponse", "No IP given (0.0.0.0) - return")
        return false;
    }
    String destIP = packet->getMember("srcIP");
    String destPort = packet->getMember("srcPort");
    String id = packet->getMember("id");
    String serviceIP = serviceData.ip.toString();
    String serviceMAC = serviceData.MAC;
    String servicePort = String(serviceData.port);

    logInfo("sendSearchResponse", "Send response to " + destIP + ", Port: " + destPort);

    String workLoad = "{\"type\": \"response\",";
    workLoad += "\"serviceName\" : \"" + serviceData.serviceName + "\",";
    workLoad += "\"id\": \"" + id + "\",";
    workLoad += "\"ip\" : \"" + serviceIP + "\",";
    workLoad += "\"port\" : \"" + servicePort + "\"";
    if(serviceMAC != "")
    {
        workLoad += ", \"mac\" : \"" + serviceIP + "\"";
    }
    workLoad += "}";

    IPAddress dest;
    dest.fromString(destIP);
    logTrace("sendSearchResponse", "Workload: " + workLoad);
    boolean result = this->_UDPM->sendUdpMessage(workLoad.c_str(), dest, destPort.toInt());

    return result;
}

short ServiceHandlerV2::getIsServicelocked(String serviceName)
{
    boolean serviceInActiveQueue = this->isSerivceInActiveSearches(serviceName);
    boolean serviceInBlockedQueue = this->isServiceBlocked(serviceName);

    if(serviceInActiveQueue)
    {
        return 1;
    }
    
    if(serviceInBlockedQueue)
    {
        return 2;
    }

    return 0;
}

void ServiceHandlerV2::handleRequest(DynamicJsonDocument *packet)
{
    String requestedServiceName = packet->getMember("serviceName");

    if(requestedServiceName.length() == 0)
    {
        //Servicename is to short (no Service given)
        logDebug("handleRequest", "Servicename is too short Name: " + String(requestedServiceName) + " - return");
        return;
    }
    else
    {
        Service requestedService;
        requestedService.serviceName = requestedServiceName;
        boolean serviceExistInternal = this->checkForSelfOfferedService(requestedService);
        boolean serviceExistExternal = this->checkForExternalService(requestedService);

        boolean sendResult = false;
        if(serviceExistInternal && serviceExistExternal)
        {
            //Both exist
            #ifdef preferExternalService
                logDebug("handleRequest", "Int. and Ext. Service existing - prefer External - send Response")
                requestedService = this->getService(requestedServiceName, serviceExistExternal);
                sendResult = this->sendSearchResponse(requestedService, packet);
            #elif preferInternalService
                logDebug("handleRequest", "Int. and Ext. Service existing - prefer Internal - send Response")
                requestedService = this->getService(requestedServiceName, false, false);
                sendResult = this->sendSearchResponse(requestedService, packet);
            #endif
        }
        else if(serviceExistExternal || serviceExistInternal)
        {
            if(serviceExistExternal)
            {
                logDebug("handleRequest", "External Service found - send response")
                requestedService = this->getService(requestedServiceName, serviceExistExternal);

                IPAddress seIP = requestedService.ip;
                int port = requestedService.port;
                logDebug("handleRequest", "Send Service: IP: " + seIP.toString() + ", Port: " + String(port));
                sendResult = this->sendSearchResponse(requestedService, packet);
            }
            else
            {
                logDebug("handleRequest", "Internal Service found - send response")
                requestedService = this->getService(requestedServiceName, false, false);
                sendResult = this->sendSearchResponse(requestedService, packet);
            }
        }
        else
        {
            logDebug("handleRequest", "Request no answer - No matching service found")
            return;
        }

        if(sendResult)
        {
            logInfo("handleRequest", "Response successfully sended!")
        }
        else
        {
            logError("handlerequest", "Error while sending response!")
        }
        return;
    }
}

void ServiceHandlerV2::handleResponse(DynamicJsonDocument *packet)
{
    String newServiceIP = packet->getMember("ip");
    String newServiceName = packet->getMember("serviceName");
    String newServicePort = packet->getMember("port");
    String newServiceMAC = "n.A";

    logDebug("handleResponse", "Handle Response: Data: New Serv. IP: " + newServiceIP + ", Port: " + newServicePort)
    if(packet->containsKey("mac"))
    {
        String temp = packet->getMember("mac");
        newServiceMAC = temp;
    }
    String requestID = packet->getMember("id");

    searchRequest* requestedService = getActiveSearchRequest(newServiceName);

    if(requestedService == NULL)
    {
        logError("handleResponse", "Error while fetching requestedService from queue!")
        return;
    }

    if(requestID == requestedService->searchID)
    {
        logDebug("handleResponse", "ID verified - add new Service")
        IPAddress nIPAddress;
        if(nIPAddress.isValid(newServiceIP))
        {
            logDebug("handleResponse", "Received IP Address is valid!")
            if(newServicePort.toInt() > 0 && newServicePort.toInt() < 65535)
            {
                logDebug("handleResponse", "Received Port is valid!")
                boolean result = nIPAddress.fromString(newServiceIP);

                if(result)
                {   
                    requestedService->serviceResult.ip = nIPAddress;
                    requestedService->serviceResult.port = newServicePort.toInt();
                    requestedService->serviceResult.serviceName = newServiceName;
                    requestedService->serviceResult.MAC = newServiceMAC;

                    logDebug("handleResponse", "IP Address successfully converted")
                    logInfo("handleResponse", "Add Service " + newServiceName + ", IP: " + newServiceIP + ", Port: " + newServicePort + ", MAC: " + newServiceMAC);
                    boolean serviceAdded;
                    if(_WiFi->getLocalIP() == nIPAddress.toString())
                    {
                        requestedService->serviceResult.external = true;
                        serviceAdded = this->addNewInternalService(requestedService);
                    }
                    else
                    {
                        requestedService->serviceResult.external = false;
                        serviceAdded = this->addNewExternalService(requestedService);
                    }
                    
                    if(serviceAdded)
                    {
                        logInfo("handleResponse", "Service successfully added - remove request");
                        this->removeActiveSearchRequest(requestedService);
                        return;
                    }
                    else
                    {
                        logError("handleResponse", "Error while adding Service! - Block for certain time...");
                        searchRequest block;
                        block.startTimestamp = millis();
                        block.requestedServiceName = requestedService->requestedServiceName;

                        this->blockedSearches.add(block);
                        this->removeActiveSearchRequest(requestedService);
                    }
                }
                else
                {
                    logError("handleResponse", "Error while converting IP Address!")
                    return;
                }
            }
            else
            {
                logError("handleResponse", "Received port is not valid!")
                return;
            }
        }
        else
        {
            logError("handleResponse", "The received IP Address is not valid!")
            return;
        }
    }
    else
    {
        logError("handleResponse", "ID of received Packet is not matching! ID: " + requestID + " != " + requestedService->searchID)
        return;
    }
    return;
}

boolean ServiceHandlerV2::addNewInternalService(searchRequest *response)
{
    String serviceName = response->requestedServiceName;
    String port = String(response->serviceResult.port);
    String filename = String(servicesPath) + "self.json";
    boolean added = this->_FM->appendJsonKey(filename.c_str(), serviceName.c_str(), port.c_str());

    if(added)
    {
        logInfo("addNewInternalService", "Internal service successfully added")
        return true;
    }
    else
    {
        logError("addNewInternalService", "Error while adding new internal Service!")
        return false;
    }
}

boolean ServiceHandlerV2::addNewInternalServiceMan(String serviceName, int port)
{
    searchRequest newService;
    newService.serviceResult.external = false;
    newService.serviceResult.port = port;
    newService.requestedServiceName = serviceName;
    newService.serviceResult.serviceName = serviceName;
    return addNewInternalService(&newService);
}

boolean ServiceHandlerV2::addNewExternalServiceMan(String serviceName, IPAddress ip, int port, String MAC)
{
    searchRequest newService;
    newService.serviceResult.external = false;
    newService.serviceResult.port = port;
    newService.requestedServiceName = serviceName;
    newService.serviceResult.serviceName = serviceName;
    newService.serviceResult.ip = ip;
    newService.serviceResult.MAC = MAC;
    return addNewExternalService(&newService);
}

boolean ServiceHandlerV2::addNewExternalService(searchRequest *response)
{
    String serviceName = response->requestedServiceName;
    String filename = servicesPath + serviceName + ".json";

    if(filename.length() > 31)
    {
        logError("addNewExternalService", "Filename is to long!")
        return false;
    }

    boolean fileCreated = this->_FM->createFile(filename.c_str());

    if(!fileCreated)
    {
        logError("addNewExternalService", "Error while creating file! - check FS");
        return false;
    }

    boolean ipAppended = this->_FM->appendJsonKey(filename.c_str(), "ip", response->serviceResult.ip.toString().c_str());
    boolean portAppended = this->_FM->appendJsonKey(filename.c_str(), "port", String(response->serviceResult.port).c_str());

    if(response->serviceResult.MAC != "n.A" && response->serviceResult.MAC != "" && response->serviceResult.MAC != "n.A" && response->serviceResult.MAC != "n.S")
    {
        boolean macAppended = this->_FM->appendJsonKey(filename.c_str(), "mac", String(response->serviceResult.MAC).c_str());

        if(!macAppended)
        {
            logError("addNewExternalService", "Erro while appending MAC!")
            this->_FM->delJsonKeyFromFile(filename.c_str(), "mac");
        }
    }

    if(ipAppended && portAppended)
    {
        logInfo("addNewExternalService", "New Service successfully created!");
        return true;
    }
    else
    {
        logError("addNewExternalService", "Error while adding new Service! - IP: " + String(ipAppended) + ", Port: " + String(portAppended));
        return false;
    }
}

void ServiceHandlerV2::begin()
{
    logDebug("begin", "Start Servicehandler - NetworkIdent on port " + String(networkIdentPort))
    if(this->_UDPM->begin())
    {
        logDebug("begin", "NetworkIdent successfully started...")
        this->networkIdentActive = true;
    }
    else
    {
        logError("begin", "Error while starting NetworkIdent!");
    }
}

void ServiceHandlerV2::stop()
{
    logDebug("stop", "Stopping Servicehandler")
    this->_UDPM->stop();
    logInfo("stop", "ServiceHandler stopped")
}

void ServiceHandlerV2::run()
{
    if(this->networkIdentActive)
    {
        this->_UDPM->run();
        this->checkForRequests();
        this->handleBlockedQueue();
        this->handleActiveQueue();
    }

    if(_WiFi->getWifiIsConnected())
    {
        if(!networkIdentActive)
        {
            static unsigned long lastCall = 0;

            if((lastCall + 15000) < millis())
            {
                this->begin();
                lastCall = millis();
            }
        }
    }
    
}

void ServiceHandlerV2::handleBlockedQueue()
{
    int lengthOfList = blockedSearches.size();

    if(lengthOfList == 0)
    {
        return;
    }

    for(int i = 0; i < lengthOfList; i++)
    {
        searchRequest req = blockedSearches.get(i);
        unsigned long unblocktime = req.startTimestamp + long(blockedTime);
        if(unblocktime <= millis())
        {
            blockedSearches.remove(i);
        }
    }
}

void ServiceHandlerV2::handleActiveQueue()
{
    int lengthOfList = activeSearches.size();

    if(lengthOfList == 0)
    {
        return;
    }

    for(int i = 0; i < lengthOfList; i++)
    {
        searchRequest req = activeSearches.get(i);
        unsigned long timeout = req.startTimestamp + long(activeTimeout);
        if(timeout <= millis())
        {
            activeSearches.remove(i);
        }
    }
}


boolean ServiceHandlerV2::changeExistingService(String serviceName, IPAddress ip, int port, String mac)
{
    if(!this->checkForService(serviceName))
    {
        logInfo("changeExistingService", "Cant change Service! - Service " + serviceName + " does not exist!")
        return false;
    }

    //check if ip is local
    if(ip.toString() == "127.0.0.1" || ip == WiFi.localIP())
    {
        this->removeService(serviceName, true);
        return this->addNewInternalServiceMan(serviceName, port);
    }
    else
    {
        this->removeService(serviceName, false);
        return this->addNewExternalServiceMan(serviceName, ip, port, mac);
    }
}

boolean ServiceHandlerV2::addService(String serviceName, IPAddress ip, int port, String mac)
{
    if(this->checkForService(serviceName))
    {
        logInfo("changeExistingService", "Cant add Service! - Service " + serviceName + " already exist!")
        return false;
    }

    //check if ip is local
    if(ip.toString() == "127.0.0.1" || ip == WiFi.localIP())
    {
        return this->addNewInternalServiceMan(serviceName, port);
    }
    else
    {
        return this->addNewExternalServiceMan(serviceName, ip, port, mac);
    }
}

void ServiceHandlerV2::stopSearch(String serviceName)
{
    boolean serviceIsInSearch = isSerivceInActiveSearches(serviceName);

    if(serviceIsInSearch)
    {
        searchRequest req;
        req.requestedServiceName = serviceName;
        this->removeActiveSearchRequest(&req);
    }
}