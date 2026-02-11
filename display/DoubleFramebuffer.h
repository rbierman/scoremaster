#pragma once

#include <vector>
#include <cstdint>
#include <mutex>

class DoubleFramebuffer {
private:
    int width, height;
    std::vector<uint8_t> bufferA;
    std::vector<uint8_t> bufferB;

    uint8_t* backBuffer;  // Logic draws here
    uint8_t* frontBuffer; // Displays read from here

    mutable std::mutex flipMutex; // 'mutable' allows locking in const functions

public:
    DoubleFramebuffer(int w, int h);

    // --- DRAWING INTERFACE ---
    void clearBack() const;

    // --- SYSTEM INTERFACE ---
    void swap();
    uint8_t* getBackData();
    const uint8_t* getFrontData() const;

    [[nodiscard]] int getWidth() const { return width; }
    [[nodiscard]] int getHeight() const { return height; }
};
