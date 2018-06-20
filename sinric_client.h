/*
Sachin Deodhar (deodharsachin@gmail.com)
*/
#ifndef _SINRIC_CLIENT_H_
#define _SINRIC_CLIENT_H_

#include <WebSocketsClient.h>
#include "ArduinoJson-v5.13.2.h"

#define SINRIC_IOT_SERVER_URL ("iot.sinric.com")
#define SINRIC_IOT_SERVER_PORT (80)
#define SINRIC_RECONNECT_INTERVAL (5000)
#define HEARTBEAT_INTERVAL 300000 // In milli seconds corresponding to 5 mins

typedef void (*SetPowerState)(String deviceID, bool status);

class SinricClient: public WebSocketsClient {
private:
    bool bConnected;
    uint64_t prevHBTimestamp;

public:
    SinricClient();
    ~SinricClient();
    SetPowerState pSetPowerStateCb;
    void setup(const char *apikey, SetPowerState powerStateCb);
    bool isSinricConnected(void);
    void setSinricConnected(bool status);
    void setPowerStateOnServer(String deviceID, bool status);
    void sendHeartBeatStatus(void);
};

#endif //_SINRIC_CLIENT_H_
