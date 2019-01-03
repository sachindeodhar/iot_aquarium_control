/*
Sachin Deodhar (deodharsachin@gmail.com)
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <StreamString.h>
#include <Ticker.h>


#include "ntp_clock_sync.h"
#include "sinric_client.h"

ESP8266WiFiMulti WiFiMulti;

Ticker oneSecTimer;
NTPClockSync clockSync;
SinricClient sinricClient;

#define SinricApiKey "<SINRIC API Key>"
#define MySSID "<Wifi SSID>"
#define MyWifiPassword "<Wifi Password>"

#define FISHTANK_LIGHT_DEV_ID ("<Device ID for light>")
#define FISHTANK_FILTER_DEV_ID ("<Device ID for filter>")

bool bInitStatus = false;

#define MAX_SEC_COUNT (3600) // In seconds corresponding to 1 hour
uint8_t sec_count = 0;
unsigned long timestamp = 0;

typedef enum {
    FISHTANK_DEV_FILTER_STATUS = 0,
    FISHTANK_DEV_LIGHT_STATUS = 1,
    FISHTANK_DEV_FILTER = 4,
    FISHTANK_DEV_LIGHT = 5,
} fish_tank_dev_t;

typedef struct {
    uint32_t onTime;
    uint32_t offTime;
    fish_tank_dev_t dev_type;
} device_schedule_t;

#define MAX_SCHEDULES (3)

device_schedule_t dev_schedules[MAX_SCHEDULES] = {
    { 28800, 46800, FISHTANK_DEV_LIGHT }, //08:00 - ON, 13:00 - OFF (Light)
    { 61200, 79200, FISHTANK_DEV_LIGHT }, //17:00 - ON, 22:00 - OFF (Light)
    { 18000, 0, FISHTANK_DEV_FILTER }, //00:00 - OFF, 05:00 - ON (Filter)
};

uint8_t fish_tank_dev_status[2] = {false, false};

void initDevice(fish_tank_dev_t dev) {
    pinMode(dev, OUTPUT);

    switch(dev) {
        case FISHTANK_DEV_FILTER:
            digitalWrite(dev, HIGH);
            fish_tank_dev_status[FISHTANK_DEV_FILTER_STATUS] = true;
            break;
        case FISHTANK_DEV_LIGHT:
            digitalWrite(dev, HIGH);
            fish_tank_dev_status[FISHTANK_DEV_LIGHT_STATUS] = false;
            break;
    }
}

void setDevicePowerState(fish_tank_dev_t dev, bool status) {
    if (dev == FISHTANK_DEV_LIGHT)
    {
        if (true == status) {
            Serial.printf("Turn on\n");
            digitalWrite(FISHTANK_DEV_LIGHT, LOW);
            fish_tank_dev_status[FISHTANK_DEV_LIGHT_STATUS] = true;
        } else {
            Serial.printf("Turn off\n");
            digitalWrite(FISHTANK_DEV_LIGHT, HIGH);
            fish_tank_dev_status[FISHTANK_DEV_LIGHT_STATUS] = false;
        }
        sinricClient.setPowerStateOnServer(FISHTANK_LIGHT_DEV_ID,
                    fish_tank_dev_status[FISHTANK_DEV_LIGHT_STATUS]);
    }
    else if (dev == FISHTANK_DEV_FILTER)
    {
        if (true == status) {
            Serial.printf("Turn on\n");
            digitalWrite(FISHTANK_DEV_FILTER, HIGH);
            fish_tank_dev_status[FISHTANK_DEV_FILTER_STATUS] = true;
        } else {
            Serial.printf("Turn off\n");
            digitalWrite(FISHTANK_DEV_FILTER, LOW);
            fish_tank_dev_status[FISHTANK_DEV_FILTER_STATUS] = false;
        }
        sinricClient.setPowerStateOnServer(FISHTANK_FILTER_DEV_ID,
                    fish_tank_dev_status[FISHTANK_DEV_FILTER_STATUS]);
    }
}

void setPowerStatusCb(String deviceId, bool status) {
    if (deviceId == FISHTANK_LIGHT_DEV_ID)
    {
        Serial.printf("Aquarium Light [%s]\n", deviceId.c_str());
        setDevicePowerState(FISHTANK_DEV_LIGHT, status);
    }
    else if (deviceId == FISHTANK_FILTER_DEV_ID)
    {
        Serial.printf("Aquarium Filter [%s]\n", deviceId.c_str());
        setDevicePowerState(FISHTANK_DEV_FILTER, status);
    }
    else {
        Serial.printf("Turn on for unknown device id: [%s]\n", deviceId.c_str());
    }
}

void onSecTimeout()
{
    if(sinricClient.isSinricConnected()) {
        int indx = 0;
        unsigned long ts = timestamp % 86400L;
        for(indx = 0; indx < MAX_SCHEDULES; indx++) {
            if(dev_schedules[indx].onTime == ts) {
                Serial.printf("[Timer]: Setting [%d] ON\n",
                                dev_schedules[indx].dev_type);
                setDevicePowerState(dev_schedules[indx].dev_type, true);
            } else if (dev_schedules[indx].offTime == ts) {
                Serial.printf("[Timer]: Setting [%d] OFF\n",
                                dev_schedules[indx].dev_type);
                setDevicePowerState(dev_schedules[indx].dev_type, false);
            }
        }
    }
    clockSync.printTimeStamp(timestamp);
    timestamp++;

    if(sec_count >= MAX_SEC_COUNT || clockSync.isNTPTimeSynced() == false) {
        sec_count = 0;
        clockSync.requestNTPTime();
    } else {
        sec_count++;
    }
}

void initWifi(void) {
    WiFiMulti.addAP(MySSID, MyWifiPassword);
    Serial.printf("Connecting to Wifi: [%s]\n", MySSID);

    // Waiting for Wifi connect
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(500);
        Serial.printf(".");
    }
    if(WiFiMulti.run() == WL_CONNECTED) {
        Serial.print("\nConnected: IP address: ");
        Serial.println(WiFi.localIP());
    }
}

void setup() {
    Serial.begin(115200);

    initWifi();
    initDevice(FISHTANK_DEV_LIGHT);
    initDevice(FISHTANK_DEV_FILTER);

    sinricClient.setup(SinricApiKey, setPowerStatusCb);
    oneSecTimer.attach(1, onSecTimeout);
    clockSync.setup();
}

void loop() {
    unsigned long ts = clockSync.checkNTPTime();
    if (ts) timestamp = ts;

    sinricClient.loop();
    if(sinricClient.isSinricConnected()) {
        sinricClient.sendHeartBeatStatus();
        if (!bInitStatus) {
            sinricClient.setPowerStateOnServer(FISHTANK_LIGHT_DEV_ID,
                              fish_tank_dev_status[FISHTANK_DEV_LIGHT_STATUS]);
            sinricClient.setPowerStateOnServer(FISHTANK_FILTER_DEV_ID,
                              fish_tank_dev_status[FISHTANK_DEV_FILTER_STATUS]);
            bInitStatus = true;
        }
    }
}
