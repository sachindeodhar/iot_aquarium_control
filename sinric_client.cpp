/*
Sachin Deodhar (deodharsachin@gmail.com)
*/
#include <Arduino.h>
#include <StreamString.h>

#include "sinric_client.h"

static SinricClient *sinricClient = NULL;

static void WebSocketClientEventCb(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED: {
            if (sinricClient) sinricClient->setSinricConnected(false);
            Serial.printf("[Sinric-WS] Webservice disconnected from sinric.com!\n");
            break;
        }
        case WStype_CONNECTED: {
            if (sinricClient) sinricClient->setSinricConnected(true);
            Serial.printf("[Sinric-WS] Service connected to sinric.com at url: %s\n", payload);
            Serial.printf("Waiting for commands from sinric.com ...\n");
            break;
        }
        case WStype_TEXT: {
            Serial.printf("[Sinric-WS] get text: %s\n", payload);

            // Example payloads

            // {"deviceId": xxxx, "action": "setPowerState", value: "ON"}
            // https://developer.amazon.com/docs/device-apis/alexa-powercontroller.html

            // {"deviceId":"5b23a02fc9860121c6c4689f","action":"action.devices.commands.OnOff","value":{"on":true}}
            // For google home

            DynamicJsonBuffer jsonBuffer;
            JsonObject& json = jsonBuffer.parseObject((char*)payload);
            String deviceId = json ["deviceId"];
            String action = json ["action"];
            if(action == "setPowerState") { // Switch or Light
                String value = json ["value"];
                if(value == "ON") {
                    if (sinricClient) sinricClient->pSetPowerStateCb(deviceId, true);
                } else {
                    if (sinricClient) sinricClient->pSetPowerStateCb(deviceId, false);
                }
            }
            else if (action == "SetTargetTemperature") {
                String deviceId = json ["deviceId"];
                String action = json ["action"];
                String value = json ["value"];
            }
	    else if (action == "action.devices.commands.OnOff") {
		bool value = json ["value"]["on"];
		if (value == true) {
                    if (sinricClient) sinricClient->pSetPowerStateCb(deviceId, true);
		} else {
                    if (sinricClient) sinricClient->pSetPowerStateCb(deviceId, false);
		}
	    }
            else if (action == "test") {
                Serial.printf("[Sinric-WS] received test command from sinric.com");
            }
            break;
        }
        case WStype_BIN: {
            Serial.printf("[Sinric-WS] get binary length: %u\n", length);
            break;
        }
    }
}

SinricClient::SinricClient() {
    sinricClient = this;
    prevHBTimestamp = 0;
}

SinricClient::~SinricClient() {
    sinricClient = NULL;
}

void SinricClient::setup(const char *apikey, SetPowerState powerStateCb) {
    begin(SINRIC_IOT_SERVER_URL, SINRIC_IOT_SERVER_PORT, "/");
    onEvent(WebSocketClientEventCb);
    setAuthorization("apikey", apikey);
    setReconnectInterval(SINRIC_RECONNECT_INTERVAL);
    pSetPowerStateCb = powerStateCb;
}

bool SinricClient::isSinricConnected(void) {
    return bConnected;
}

void SinricClient::setSinricConnected(bool status) {
    bConnected = status;
}

void SinricClient::setPowerStateOnServer(String deviceID, bool status) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    root["deviceId"] = deviceID;
    root["action"] = "setPowerState";

    if (true == status) {
        root["value"] = "ON";
    } else {
        root["value"] = "OFF";
    }

    Serial.printf("Update Server Power State [%s]\n", status ? "ON" : "OFF");

    StreamString databuf;
    root.printTo(databuf);

    sendTXT(databuf);
}

void SinricClient::sendHeartBeatStatus(void) {
    uint64_t now = millis();
    if((now - prevHBTimestamp) > HEARTBEAT_INTERVAL) {
        prevHBTimestamp = now;
        sendTXT("H");
        Serial.printf("Sending Keep Alive Heartbeat\n");
    }
}
