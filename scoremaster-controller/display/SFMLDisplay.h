#pragma once

#include "IDisplay.h"
#include <SFML/Graphics/RenderWindow.hpp>

class DoubleFramebuffer;

class SFMLDisplay : public IDisplay {
    sf::RenderWindow window;

    const unsigned int PIXEL_SIZE = 4; // Size of each simulated pixel
    const unsigned int PIXEL_GAP = 1;  // Gap between simulated pixels

public:
    explicit SFMLDisplay(DoubleFramebuffer& buffer);

    void output() override;
    bool isOpen() const;
    sf::RenderWindow& getWindow() { return window; }
};