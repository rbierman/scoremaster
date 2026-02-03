#include "ColorLightDisplay.h"
#include "DoubleFramebuffer.h" // Needed for dfb
#include <iostream>
#include <utility>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>

// Constants based on the ColorLight protocol in the FPP source
#define CL_SYNC_PACKET_TYPE 0x01 // sync memory to outputs
#define CL_SYNC_PACKET_SIZE 112
#define CL_SYNC_DATA_OFFSET 13

#define CL_PACKET_TYPE_OFFSET 12
#define CL_PACKET_DATA_OFFSET 13

#define CL_BRIG_PACKET_TYPE 0x0A // brightness
#define CL_BRIG_PACKET_SIZE 77

#define CL_PIXL_PACKET_TYPE 0x55 // row data
#define CL_PIXL_HEADER_SIZE 8

// We can fit about 497 pixels in one Ethernet frame (MTU 1500)
#define CL_MAX_PIXL_PER_PACKET 497

ColorLightDisplay::ColorLightDisplay(std::string  interface, DoubleFramebuffer& buffer)
    : IDisplay(buffer), m_interface(std::move(interface)) {
    setupSocket();
}

ColorLightDisplay::~ColorLightDisplay() {
    if (m_sockfd >= 0) close(m_sockfd);
}

void ColorLightDisplay::output() {
    uint8_t packet[1500];
    const int width = dfb.getWidth();
    const int height = dfb.getHeight();
    const uint8_t* framebuffer_data = dfb.getFrontData();

    sendBrightness(255);

    for (int rowNumber = 0; rowNumber < height; rowNumber++) {
        int pixelsSent = 0;

        while (pixelsSent < width) {
            memset(packet, 0, 20); // Clear header space

            // Ethernet Header
            memcpy(packet, destMac, 6);
            memcpy(packet + 6, srcMac, 6);
            packet[12] = 0x55; // Data Packet Type

            const int numPixels = std::min(CL_MAX_PIXL_PER_PACKET, width - pixelsSent);

            // ColorLight Header (Bytes 14-19)
            int dataIndex = CL_PACKET_DATA_OFFSET;
            packet[dataIndex++] = (rowNumber >> 8) & 0xFF; // Row number MSB
            packet[dataIndex++] = rowNumber & 0xFF;        // Row number LSB
            packet[dataIndex++] = (pixelsSent >> 8) & 0xFF; // Offset MSB
            packet[dataIndex++] = pixelsSent & 0xFF;        // Offset LSB
            packet[dataIndex++] = (numPixels >> 8) & 0xFF;  // Count MSB
            packet[dataIndex++] = numPixels & 0xFF;         // Count LSB
            packet[dataIndex++] = 0x08;
            packet[dataIndex++] = 0x88;

            // Copy RGB data directly from our buffer into the packet starting at byte 20
            // The data from the framebuffer is RGBA, but the ColorLight panel expects BGR.
            // We need to swizzle the colors.
            const uint8_t* row_data_start = framebuffer_data + (rowNumber * width * 4) + (pixelsSent * 4);
            uint8_t* packet_data_start = packet + dataIndex;

            for (int i = 0; i < numPixels; ++i) {
                packet_data_start[i * 3 + 0] = row_data_start[i * 4 + 2]; // Blue
                packet_data_start[i * 3 + 1] = row_data_start[i * 4 + 1]; // Green
                packet_data_start[i * 3 + 2] = row_data_start[i * 4 + 0]; // Red
            }

            sendraw(packet, dataIndex + (numPixels * 3));
            pixelsSent += numPixels;
        }
    }
    sendSync();
}

void ColorLightDisplay::sendSync() {
    uint8_t packet[CL_SYNC_PACKET_SIZE] = {};
    memcpy(packet, destMac, 6);
    memcpy(packet + 6, srcMac, 6);

    packet[12] = 0x01; // Sync Type


    packet[CL_SYNC_DATA_OFFSET + 22] = 0xff;
    packet[CL_SYNC_DATA_OFFSET + 25] = 0xff;
    packet[CL_SYNC_DATA_OFFSET + 26] = 0xff;
    packet[CL_SYNC_DATA_OFFSET + 27] = 0xff;

    sendraw(packet, CL_SYNC_PACKET_SIZE);
}

void ColorLightDisplay::sendBrightness(const uint8_t brightness) {
    uint8_t packet[CL_BRIG_PACKET_SIZE] = {};
    memcpy(packet, destMac, 6);
    memcpy(packet + 6, srcMac, 6);

    packet[12] = 0x0a; // Management Type
    packet[13] = brightness;

    // Command: Set Brightness
    packet[14] = brightness;
    packet[15] = brightness;
    packet[16] = 0xff; // Hard-coded 0xff

    sendraw(packet, CL_BRIG_PACKET_SIZE);
}

void ColorLightDisplay::setupSocket() {
    m_sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (m_sockfd < 0) {
        perror("Socket creation failed. Try sudo.");
        exit(1);
    }

    ifreq ifr{};
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, m_interface.c_str(), IFNAMSIZ-1);
    if (ioctl(m_sockfd, SIOCGIFINDEX, &ifr) < 0) {
        perror("Interface lookup failed");
        exit(1);
    }

    memset(&m_socket_address, 0, sizeof(m_socket_address));
    m_socket_address.sll_family = AF_PACKET;
    m_socket_address.sll_ifindex = ifr.ifr_ifindex;
    m_socket_address.sll_halen = ETH_ALEN;

    if (bind(m_sockfd, reinterpret_cast<sockaddr *>(&m_socket_address), sizeof(m_socket_address)) == -1) {
        perror("Bind failed");
        exit(1);
    }
}

void ColorLightDisplay::sendraw(const uint8_t* data, const int len) {
    // Manually ensure the ethertype bytes are what the card expects
    // packet[12] = 0x55 (for data) or 0x01 (for sync)
    // packet[13] = sequence
    sendto(m_sockfd, data, len, 0, reinterpret_cast<sockaddr *>(&m_socket_address), sizeof(m_socket_address));
}
