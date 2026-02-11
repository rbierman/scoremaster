#pragma once
class DoubleFramebuffer;

class IDisplay {
public:
    virtual ~IDisplay() = default;

    IDisplay(DoubleFramebuffer& buffer) : dfb(buffer) {}    // The display takes the buffer and pushes it to its hardware/window
    virtual void output() = 0;
protected:
    DoubleFramebuffer& dfb;
};
