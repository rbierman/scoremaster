#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <string>
#include <vector>
#include "ScoreboardController.h"

class KeyboardSimulator {
public:
    KeyboardSimulator(ScoreboardController& controller);

    void handleInput(sf::RenderWindow& window);
    void printInstructions() const;

private:
    ScoreboardController& scoreboard;
    
    size_t homeNameIdx = 0;
    size_t awayNameIdx = 1;

    static const std::vector<std::string> TEAM_NAMES;
};
