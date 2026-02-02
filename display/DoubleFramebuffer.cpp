#include "DoubleFramebuffer.h"
#include <algorithm>

DoubleFramebuffer::DoubleFramebuffer(const int w, const int h) : width(w), height(h) {
    bufferA.resize(w * h * 4, 0);
    bufferB.resize(w * h * 4, 0);
    backBuffer = bufferA.data();
    frontBuffer = bufferB.data();
}

void DoubleFramebuffer::drawPixel(const int x, const int y, const uint8_t r, const uint8_t g, const uint8_t b) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    const int idx = (y * width + x) * 4;
    backBuffer[idx] = r;
    backBuffer[idx+1] = g;
    backBuffer[idx+2] = b;
    backBuffer[idx+3] = 255;
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