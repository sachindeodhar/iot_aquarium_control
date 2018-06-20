/*
Sachin Deodhar (deodharsachin@gmail.com)
*/
#include <Arduino.h>
#include <WiFiUdp.h>

#include "ntp_clock_sync.h"

NTPClockSync::NTPClockSync() {
  synced = false;
  timestamp = 0;
  udp = new WiFiUDP();
}

NTPClockSync::~NTPClockSync() {
  delete udp;
}

void NTPClockSync::setup(void) {
    udp->begin(NTP_UTP_PORT);
    Serial.printf("NTPClockSync::setup: UDP starte on [%d]\n", NTP_UTP_PORT);
    WiFi.hostByName(NTP_SERVER_URL, ntp_server_ip);
}

void NTPClockSync::requestNTPTime(void) {
  memset(ntp_packet_buf, 0, NTP_PACKET_HEADER_MAX_SIZE);

  ntp_packet_buf[0] = 0b11100011;   // LI, Version, Mode
  ntp_packet_buf[1] = 0;     // Stratum, or type of clock
  ntp_packet_buf[2] = 6;     // Polling Interval
  ntp_packet_buf[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  ntp_packet_buf[12]  = 49;
  ntp_packet_buf[13]  = 0x4E;
  ntp_packet_buf[14]  = 49;
  ntp_packet_buf[15]  = 52;

  udp->beginPacket(ntp_server_ip, 123); //NTP requests are to port 123
  udp->write(ntp_packet_buf, NTP_PACKET_HEADER_MAX_SIZE);
  udp->endPacket();
  Serial.printf("NTPClockSync::sendNTPPacket: sending NTP packet\n");
}

void NTPClockSync::printTimeStamp(unsigned long ts) {
  unsigned int hours = (ts % 86400L) / 3600;
  unsigned int mins = (ts % 3600) / 60;
  unsigned int secs = (ts % 60);
  Serial.printf("NTPClockSync::printTimeStamp: [%02d:%02d:%02d]\n", hours, mins, secs);
}

unsigned long NTPClockSync::checkNTPTime(void) {
  int cb = udp->parsePacket();
  if (cb) {
    udp->read(ntp_packet_buf, NTP_PACKET_HEADER_MAX_SIZE);
    unsigned long highWord = word(ntp_packet_buf[40], ntp_packet_buf[41]);
    unsigned long lowWord = word(ntp_packet_buf[42], ntp_packet_buf[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;
    timestamp = epoch + INDIA_TIME_ZONE;
    synced = true;
    Serial.printf("NTPClockSync::checkNTPTime: length [%d], Sec [%u]\n", cb, secsSince1900);
    printTimeStamp(timestamp);
    return timestamp;
  } else {
    return 0;
  }
}

bool NTPClockSync::isNTPTimeSynced(void) {
  return synced;
}
