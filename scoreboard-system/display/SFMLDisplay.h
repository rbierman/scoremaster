#pragma once

#include "IDisplay.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp> // New include
#include <SFML/Graphics/Color.hpp> // New include

class DoubleFramebuffer;

class SFMLDisplay : public IDisplay {
    sf::RenderWindow window;
    // Removed sf::Texture texture;
    // Removed sf::Sprite sprite;

    const unsigned int PIXEL_SIZE = 4; // Size of each simulated pixel
    const unsigned int PIXEL_GAP = 1;  // Gap between simulated pixels

public:
    explicit SFMLDisplay(DoubleFramebuffer& buffer);

    void output() override;
    bool isOpen() const;
    sf::RenderWindow& getWindow() { return window; }
};