#include "DoubleFramebuffer.h"
#include <algorithm>
#include <cstring>

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

DoubleFramebuffer::DoubleFramebuffer(int w, int h) : width(w), height(h) {
    bufferA.resize(w * h * 4); // 4 bytes per pixel (RGBA)
    bufferB.resize(w * h * 4);

    backBuffer = bufferA.data();
    frontBuffer = bufferB.data();
}

void DoubleFramebuffer::drawPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return; // Clipping
    }
    int offset = (y * width + x) * 4;
    backBuffer[offset + 0] = r;
    backBuffer[offset + 1] = g;
    backBuffer[offset + 2] = b;
    backBuffer[offset + 3] = 255; // Alpha
}

void DoubleFramebuffer::drawNumber(int num, int x, int y, uint8_t r, uint8_t g, uint8_t b) const {
    if (num < 0 || num > 9) return; // Only supports single digits 0-9

    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 3; ++col) {
            if (FONT[num][row][col] == 1) {
                drawPixel(x + col, y + row, r, g, b);
            }
        }
    }
}

void DoubleFramebuffer::clearBack() const {
    std::fill_n(backBuffer, (width * height * 4), 0);
}

void DoubleFramebuffer::swap() {
    std::lock_guard<std::mutex> lock(flipMutex);
    std::swap(backBuffer, frontBuffer);
}

const uint8_t* DoubleFramebuffer::getFrontData() const {
    std::lock_guard<std::mutex> lock(flipMutex);
    return frontBuffer;
}
