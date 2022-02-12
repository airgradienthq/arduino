#include "NTPClient.h"

#include <utility>

void AirGradient_Internal::NTPClient::sendNTPpacket(IPAddress &address) {
    // set all bytes in the buffer to 0
    memset(_packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    _packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    _packetBuffer[1] = 0;     // Stratum, or type of clock
    _packetBuffer[2] = 6;     // Polling Interval
    _packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    _packetBuffer[12] = 49;
    _packetBuffer[13] = 0x4E;
    _packetBuffer[14] = 49;
    _packetBuffer[15] = 52;
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    _udp.beginPacket(address, 123); //NTP requests are to port 123
    _udp.write(_packetBuffer, NTP_PACKET_SIZE);
    _udp.endPacket();
}

time_t AirGradient_Internal::NTPClient::getUtcUnixEpoch() {
    IPAddress ntpServerIP; // NTP server's ip address

    while (_udp.parsePacket() > 0); // discard any previously received packets
    _udp.flush();

    Serial.println("Transmit NTP Request");
    WiFi.hostByName(_ntpServer, ntpServerIP);
    Serial.printf("%s: %s\n", _ntpServer, ntpServerIP.toString().c_str());
    sendNTPpacket(ntpServerIP);
    uint8_t timeout = 0;
    size_t cb = 0;

    do {
        delay(10);
        cb = _udp.parsePacket();
        if (timeout > 100) { // timeout after 1000 ms
            Serial.println("No NTP Response");
            return 0;
        }
        timeout++;
    } while (cb == 0);

    _udp.read(this->_packetBuffer, NTP_PACKET_SIZE);

    unsigned long highWord = word(this->_packetBuffer[40], this->_packetBuffer[41]);
    unsigned long lowWord = word(this->_packetBuffer[42], this->_packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    time_t epoch = secsSince1900 - SEVENTIES;
    Serial.println("Epoch: " + String(epoch));
    return epoch;
}