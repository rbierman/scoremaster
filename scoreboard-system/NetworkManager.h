#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <vector>

class NetworkManager {
public:
    NetworkManager(uint16_t port);
    ~NetworkManager();

    void start();
    void stop();

private:
    uint16_t servicePort;
    std::atomic<bool> running{false};
    std::thread mDNSThread;

    void runmDNS();
};
