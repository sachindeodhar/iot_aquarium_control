/*
Sachin Deodhar (deodharsachin@gmail.com)
*/
#ifndef _NTP_CLOCK_SYNC_H_
#define _NTP_CLOCK_SYNC_H_

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define NTP_PACKET_HEADER_MAX_SIZE (48)
#define NTP_SERVER_URL ("0.in.pool.ntp.org")
#define NTP_UTP_PORT (2390)
#define INDIA_TIME_ZONE (19800)

class NTPClockSync {
  private:
    byte ntp_packet_buf[NTP_PACKET_HEADER_MAX_SIZE];
    IPAddress ntp_server_ip;
    WiFiUDP *udp;
    unsigned long timestamp;
    bool synced;
  public:
    NTPClockSync();
    ~NTPClockSync();
    void setup(void);
    void requestNTPTime(void);
    void printTimeStamp(unsigned long timestamp);
    unsigned long checkNTPTime(void);
    bool isNTPTimeSynced(void);
};

#endif //_NTP_CLOCK_SYNC_H_
