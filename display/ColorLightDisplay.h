#pragma once

#include "IDisplay.h"
#include <string>
#include <cstdint>
#include <sys/socket.h>
#include <net/if.h>
#include <linux/if_packet.h>

class ColorLightDisplay : public IDisplay {
public:
    ColorLightDisplay(std::string  interface, DoubleFramebuffer& buffer);
    ~ColorLightDisplay() override;

    void output() override;


private:
    int m_sockfd;
    std::string m_interface;
    sockaddr_ll m_socket_address;

    const uint8_t destMac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    const uint8_t srcMac[6] = {0x22, 0x22, 0x33, 0x44, 0x55, 0x66};

    void setupSocket();
    void sendBrightness(uint8_t brightness);
    void sendraw(uint8_t* data, int len);
    void sendSync();
};
