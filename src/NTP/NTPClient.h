#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <utility>
#define NTP_PACKET_SIZE 48
#define SEVENTIES 2208988800UL
#define NTP_DEFAULT_LOCAL_PORT 1515


namespace AirGradient {
#if !defined(__time_t_defined) // avoid conflict with newlib or other posix libc
    typedef unsigned long time_t;
#endif

    using byte = unsigned char;


    class NTPClient {

    public:
        NTPClient() {
            _udp.begin(NTP_DEFAULT_LOCAL_PORT);
        }
        time_t getUtcUnixEpoch();
        virtual ~NTPClient() = default;

        NTPClient(String ntpServer);

    private:
        String _ntpServer;
        WiFiUDP _udp;
        byte _packetBuffer[NTP_PACKET_SIZE]{};
        void sendNTPpacket(IPAddress &address);

    };
}