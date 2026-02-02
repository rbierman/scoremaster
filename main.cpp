#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_packet.h>
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

// 5x3 Font Definition
const int FONT[10][5][3] = {
    {{1,1,1},{1,0,1},{1,0,1},{1,0,1},{1,1,1}}, // 0
    {{0,1,0},{0,1,0},{0,1,0},{0,1,0},{0,1,0}}, // 1
    {{1,1,1},{0,0,1},{1,1,1},{1,0,0},{1,1,1}}, // 2
    {{1,1,1},{0,0,1},{1,1,1},{0,0,1},{1,1,1}}, // 3
    {{1,0,1},{1,0,1},{1,1,1},{0,0,1},{0,0,1}}, // 4
    {{1,1,1},{1,0,0},{1,1,1},{0,0,1},{1,1,1}}, // 5
    {{1,1,1},{1,0,0},{1,1,1},{1,0,1},{1,1,1}}, // 6
    {{1,1,1},{0,0,1},{0,0,1},{0,0,1},{0,0,1}}, // 7
    {{1,1,1},{1,0,1},{1,1,1},{1,0,1},{1,1,1}}, // 8
    {{1,1,1},{1,0,1},{1,1,1},{0,0,1},{1,1,1}}  // 9
};


class ColorLightDriver {
public:
    ColorLightDriver(const std::string& interface, int width, int height)
        : m_interface(interface), m_width(width), m_height(height) {
        setupSocket();
    }

    ~ColorLightDriver() {
        if (m_sockfd >= 0) close(m_sockfd);
    }

    void sendScoreboardFrame() {
        uint8_t packet[1500];
        int width = 64;
        int height = 32;

        for (int rowNumber = 0; rowNumber < height; rowNumber++) {
            int pixelsSent = 0;

            // --- STEP 1: SEND PIXEL DATA ---
            while (pixelsSent < width) {
                memset(packet, 0, 20); // Clear header space

                // Ethernet Header
                memcpy(packet, destMac, 6);
                memcpy(packet + 6, srcMac, 6);
                packet[12] = 0x55; // Data Packet Type

                int numPixels = std::min(CL_MAX_PIXL_PER_PACKET, width - pixelsSent);

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
                // No ARGB loop needed!
                memcpy(packet + dataIndex, &scoreboardBuffer[rowNumber][0][0] + (pixelsSent * 3), numPixels * 3);

                sendraw(packet, dataIndex + (numPixels * 3));
                pixelsSent += numPixels;

                // usleep(333); // 30 FPS
            }
        }
    }

    void sendSync() {
        uint8_t packet[CL_SYNC_PACKET_SIZE];
        memset(packet, 0, CL_SYNC_PACKET_SIZE);
        memcpy(packet, destMac, 6);
        memcpy(packet + 6, srcMac, 6);

        packet[12] = 0x01; // Sync Type


        packet[CL_SYNC_DATA_OFFSET + 22] = 0xff;
        packet[CL_SYNC_DATA_OFFSET + 25] = 0xff;
        packet[CL_SYNC_DATA_OFFSET + 26] = 0xff;
        packet[CL_SYNC_DATA_OFFSET + 27] = 0xff;

        sendraw(packet, CL_SYNC_PACKET_SIZE);
    }

    void sendBrightness(uint8_t brightness) {
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

    void drawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        // 1. Safety check: prevent writing outside the buffer (segmentation fault)
        if (x < 0 || x >= 64 || y < 0 || y >= 32) return;

        // 2. Set the color bytes directly
        // [y] selects the row, [x] selects the column, [0-2] selects R, G, or B
        scoreboardBuffer[y][x][0] = b;
        scoreboardBuffer[y][x][1] = g;
        scoreboardBuffer[y][x][2] = r;
    }

    /**
     * Prints a 3D RGB structure to the console.
     * Structure: screen[y][x][color]
     */
    void printScoreboard() {
        for (const auto& row : scoreboardBuffer) {
            std::string line = "";
            for (const auto& pixel : row) {
                // Extract RGB (0=r, 1=g, 2=b)
                const int r = pixel[0];
                const int g = pixel[1];
                const int b = pixel[2];

                // ANSI sequence: \x1b[38;2;R;G;Bm
                // Using "██" (two blocks) to approximate a square pixel
                line += "\x1b[38;2;" + std::to_string(b) + ";"
                                     + std::to_string(g) + ";"
                                     + std::to_string(r) + "m██";
            }
            // Output line and reset color with \x1b[0m
            std::cout << line << "\x1b[0m" << std::endl;
        }
    }

    void drawNumber(int num, int startX, int startY, uint8_t r, uint8_t g, uint8_t b) {
        for (int y = 0; y < 5; ++y) {
            for (int x = 0; x < 3; ++x) {
                if (FONT[num][y][x]) {
                    drawPixel(startX + x, startY + y, r, g, b);
                }
            }
        }
    }

private:
    int m_sockfd;
    std::string m_interface;
    int m_width, m_height;
    struct sockaddr_ll m_socket_address;
    uint8_t scoreboardBuffer[32][64][3] = {};

    const uint8_t destMac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    const uint8_t srcMac[6] = {0x22, 0x22, 0x33, 0x44, 0x55, 0x66};

    void setupSocket() {
        m_sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if (m_sockfd < 0) {
            perror("Socket creation failed. Try sudo.");
            exit(1);
        }

        struct ifreq ifr;
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

        if ((bind(m_sockfd, (struct sockaddr*)&m_socket_address, sizeof(m_socket_address))) == -1) {
            perror("Bind failed");
            exit(1);
        }


    }

    void sendraw(uint8_t* data, int len) {
        // Manually ensure the ethertype bytes are what the card expects
        // packet[12] = 0x55 (for data) or 0x01 (for sync)
        // packet[13] = sequence
        sendto(m_sockfd, data, len, 0, (struct sockaddr*)&m_socket_address, sizeof(m_socket_address));
    }
};



int main() {
    int w = 64, h = 32;
    ColorLightDriver cl("enx00e04c68012e", w, h);


    std::cout << "Starting scoreboard loop..." << std::endl;
    while(true) {
        cl.sendBrightness(255);

        // 1. Draw "H" (Home) in Red
        for(int i=2; i<7; i++) {
            cl.drawPixel(2, i, 255, 50, 50);
            cl.drawPixel(4, i, 255, 50, 50);
        }
        cl.drawPixel(3, 4, 255, 50, 50);

        // 2. Draw Home Score (e.g., 5)
        cl.drawNumber(5, 7, 2, 255, 255, 255);

        // 3. Draw ":" Separator
        cl.drawPixel(19, 3, 200, 200, 200);
        cl.drawPixel(19, 5, 200, 200, 200);

        // 4. Draw Visitor Score (e.g., 2)
        cl.drawNumber(2, 30, 2, 255, 255, 255);

        // 5. Draw "V" (Visitor) in Blue
        for(int i=2; i<5; i++) {
            cl.drawPixel(35, i, 50, 50, 255);
            cl.drawPixel(39, i, 50, 50, 255);
        }
        cl.drawPixel(36, 5, 50, 50, 255);
        cl.drawPixel(38, 5, 50, 50, 255);
        cl.drawPixel(37, 6, 50, 50, 255);

        // Send the flat buffer to the hardware
        cl.sendScoreboardFrame();
        // usleep(333);
        cl.sendSync();
        usleep(33333); // 30 FPS
        cl.printScoreboard();
        // return 0;
    }



    return 0;
}