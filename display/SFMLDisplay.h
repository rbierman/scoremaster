#pragma once

#include "IDisplay.h"
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

class DoubleFramebuffer;

class SFMLDisplay : public IDisplay {
private:
    sf::RenderWindow window;
    sf::Texture texture;
    sf::Sprite sprite;

public:
    explicit SFMLDisplay(DoubleFramebuffer& buffer);

    void output() override;
};
