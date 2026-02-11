//
// Created by ron on 2026-02-01.
//

#include "SFMLDisplay.h"
#include "DoubleFramebuffer.h"
#include <iostream>
#include <SFML/Window/Event.hpp>


SFMLDisplay::SFMLDisplay(DoubleFramebuffer& buffer)
    : IDisplay(buffer)
{
    auto w = static_cast<unsigned int>(dfb.getWidth());
    auto h = static_cast<unsigned int>(dfb.getHeight());

    // Calculate window size based on pixel size and gap
    unsigned int windowWidth = w * (PIXEL_SIZE + PIXEL_GAP);
    unsigned int windowHeight = h * (PIXEL_SIZE + PIXEL_GAP);

    window.create(sf::VideoMode({windowWidth, windowHeight}), "Preview");
    // Removed sprite.setScale(...) as we draw individual shapes now

    std::cout << "SFML initialized at " << w << "x" << h << std::endl;
}

void SFMLDisplay::output() {
    if (!window.isOpen()) {
        return; // Don't do anything if the window is closed
    }

    while (const auto event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
            window.close();
    }

    window.clear(); // Clear to black

    if (!window.isOpen()) { // Check again in case it was closed in event loop
        return;
    }

    // Get pixel data from framebuffer
    const uint8_t* pixels = dfb.getFrontData();
    unsigned int fbWidth = dfb.getWidth();
    unsigned int fbHeight = dfb.getHeight();

    // Create a shape to represent each pixel (can be reused for efficiency if needed)
    sf::RectangleShape pixelShape(sf::Vector2f(static_cast<float>(PIXEL_SIZE), static_cast<float>(PIXEL_SIZE)));

    for (unsigned int y = 0; y < fbHeight; ++y) {
        for (unsigned int x = 0; x < fbWidth; ++x) {
            unsigned int index = (y * fbWidth + x) * 4; // RGBA format

            uint8_t b = pixels[index + 0];
            uint8_t g = pixels[index + 1];
            uint8_t r = pixels[index + 2];
            uint8_t a = pixels[index + 3]; // Alpha not used for display color

            pixelShape.setFillColor(sf::Color(r, g, b, a));
            pixelShape.setPosition(
                sf::Vector2f(
                    static_cast<float>(x * (PIXEL_SIZE + PIXEL_GAP)),
                    static_cast<float>(y * (PIXEL_SIZE + PIXEL_GAP))
                )
            );
            window.draw(pixelShape);
        }
    }

    window.display();
}

bool SFMLDisplay::isOpen() const {
    return window.isOpen();
}
