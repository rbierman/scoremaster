#include "KeyboardSimulator.h"
#include <SFML/Window/Event.hpp>
#include <iostream>

const std::vector<std::string> KeyboardSimulator::TEAM_NAMES = {
    "MAMBAS", "BREAKERS", "EAGLES", "TIGERS", "SHARKS", "WOLVES", "LIONS", "STARS"
};

KeyboardSimulator::KeyboardSimulator(ScoreboardController& controller)
    : scoreboard(controller) {
    scoreboard.setHomeTeamName(TEAM_NAMES[homeNameIdx]);
    scoreboard.setAwayTeamName(TEAM_NAMES[awayNameIdx]);
}

void KeyboardSimulator::handleInput(sf::RenderWindow& window) {
    while (const std::optional event = window.pollEvent()) {
        if (event->getIf<sf::Event::Closed>()) {
            window.close();
        } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            switch (keyPressed->code) {
                case sf::Keyboard::Key::Space:
                    scoreboard.toggleClock();
                    break;
                case sf::Keyboard::Key::C:
                    scoreboard.setClockMode(ClockMode::Clock);
                    break;
                case sf::Keyboard::Key::S:
                    scoreboard.setClockMode(ClockMode::Stopped);
                    break;
                case sf::Keyboard::Key::R:
                    scoreboard.setClockMode(ClockMode::Running);
                    break;
                case sf::Keyboard::Key::Num1:
                    scoreboard.addHomeScore(1);
                    break;
                case sf::Keyboard::Key::Num2:
                    scoreboard.addAwayScore(1);
                    break;
                case sf::Keyboard::Key::Num3:
                    scoreboard.addHomeShots(1);
                    break;
                case sf::Keyboard::Key::Num4:
                    scoreboard.addAwayShots(1);
                    break;
                case sf::Keyboard::Key::H:
                    scoreboard.addHomePenalty(120, 22);
                    break;
                case sf::Keyboard::Key::A:
                    scoreboard.addAwayPenalty(120, 33);
                    break;
                case sf::Keyboard::Key::P:
                    scoreboard.nextPeriod();
                    break;
                case sf::Keyboard::Key::X:
                    scoreboard.resetGame();
                    break;
                case sf::Keyboard::Key::T:
                    homeNameIdx = (homeNameIdx + 1) % TEAM_NAMES.size();
                    scoreboard.setHomeTeamName(TEAM_NAMES[homeNameIdx]);
                    break;
                case sf::Keyboard::Key::Y:
                    awayNameIdx = (awayNameIdx + 1) % TEAM_NAMES.size();
                    scoreboard.setAwayTeamName(TEAM_NAMES[awayNameIdx]);
                    break;
                default:
                    break;
            }
        }
    }
}

void KeyboardSimulator::printInstructions() const {
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "SIMULATOR CONTROLS:" << std::endl;
    std::cout << "  [Space] Toggle Start/Stop" << std::endl;
    std::cout << "  [1]     Home Score+" << std::endl;
    std::cout << "  [2]     Away Score+" << std::endl;
    std::cout << "  [3]     Home Shots+" << std::endl;
    std::cout << "  [4]     Away Shots+" << std::endl;
    std::cout << "  [H]     Home Penalty (2:00)" << std::endl;
    std::cout << "  [A]     Away Penalty (2:00)" << std::endl;
    std::cout << "  [P]     Next Period" << std::endl;
    std::cout << "  [T]     Toggle Home Team Name" << std::endl;
    std::cout << "  [Y]     Toggle Away Team Name" << std::endl;
    std::cout << "  [C]     Clock Mode (System Time)" << std::endl;
    std::cout << "  [R]     Resume Running" << std::endl;
    std::cout << "  [S]     Stop Clock" << std::endl;
    std::cout << "  [X]     Reset Game" << std::endl;
    std::cout << "------------------------------------------" << std::endl;
}
