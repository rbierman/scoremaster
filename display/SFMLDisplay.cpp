//
// Created by ron on 2026-02-01.
//

#include <SFML/Graphics.hpp>
#include <iostream>

#include "IDisplay.h"
#include "DoubleFramebuffer.h"

class SFMLDisplay : public IDisplay {
    sf::RenderWindow window;
    sf::Texture texture;
    sf::Sprite sprite;

public:
    SFMLDisplay(DoubleFramebuffer& buffer) : IDisplay(buffer), sprite(texture) { // Bind sprite to texture immediately {
        auto w = static_cast<unsigned int>(dfb.getWidth());
        auto h = static_cast<unsigned int>(dfb.getHeight());

        // SFML 3: VideoMode takes a Vector2u
        window.create(sf::VideoMode({w * 2, h * 2}), "Preview");

        // SFML 3: create() is now resize() or handled via texture.resize()
        // If resize() doesn't work, check your SFML version; create() was changed.
        texture.resize({w, h});

        // SFML 3: setScale takes a Vector2f
        sprite.setScale({2.0f, 2.0f});

        std::cout << "SFML initialized at " << w << "x" << h << std::endl;
    }

    void output() override {
        // We only pull the data that is marked as "Front"
        texture.update(dfb.getFrontData());

        window.clear();
        window.draw(sprite);
        window.display();
    }
};
