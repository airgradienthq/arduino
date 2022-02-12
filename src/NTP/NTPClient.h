#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <utility>

#define NTP_PACKET_SIZE 48
#define SEVENTIES 2208988800UL
#define NTP_DEFAULT_LOCAL_PORT UINT16_C(1515)


namespace AirGradient_Internal {
#if !defined(__time_t_defined) // avoid conflict with newlib or other posix libc
    typedef unsigned long time_t;
#endif

    using byte = unsigned char;


    class NTPClient {

    public:

        /**
         * Get current Time in UTC in UnixEpoch
         * @return
         */
        time_t getUtcUnixEpoch();

        virtual ~NTPClient() = default;

        NTPClient(const char *ntpServer, uint16_t udpPort) : _ntpServer(ntpServer) {
            _udp.begin(udpPort);
        }

        NTPClient(const char *ntpServer) : NTPClient(ntpServer, NTP_DEFAULT_LOCAL_PORT) {}

    private:

        const char *_ntpServer;
        WiFiUDP _udp;
        byte _packetBuffer[NTP_PACKET_SIZE]{};

        void sendNTPpacket(IPAddress &address);

    };
}