#pragma once
#include <Arduino.h>

struct Service {
    String serviceName;
    boolean external = true;
    IPAddress ip;
    int port;
    String MAC = "";
};

struct searchRequest {
    short status = -1;
    String searchID = "-1";
    String requestedServiceName = "";
    unsigned long startTimestamp;
    Service serviceResult;
};