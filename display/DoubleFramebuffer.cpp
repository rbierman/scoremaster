#include "DoubleFramebuffer.h"
#include <algorithm>

DoubleFramebuffer::DoubleFramebuffer(int w, int h) : width(w), height(h) {
    bufferA.resize(w * h * 4); // 4 bytes per pixel (RGBA)
    bufferB.resize(w * h * 4);

    backBuffer = bufferA.data();
    frontBuffer = bufferB.data();
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

uint8_t* DoubleFramebuffer::getBackData() {
    return backBuffer;
}
