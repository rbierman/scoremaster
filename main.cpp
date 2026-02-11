#include <iostream>
#include <vector>
#include <string>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Sleep.hpp>
#include <chrono>

#include "display/DoubleFramebuffer.h"
#include "display/SFMLDisplay.h"
#include "display/ColorLightDisplay.h"
#include "ScoreboardController.h"
#include "ScoreboardRenderer.h"
#include "CommandLineArgs.h"
#include "ResourceLocator.h"

int main(int argc, char* argv[]) {

    CommandLineArgs args(argc, argv);

    if (args.showHelp()) {
        args.printHelp(argv[0]);
        return 0;
    }

    int w = 384, h = 160;

    DoubleFramebuffer dfb(w, h);
    std::vector<IDisplay*> displays;

    SFMLDisplay* sfmlDisplay = nullptr;
    if (args.enableSFML()) {
        sfmlDisplay = new SFMLDisplay(dfb);
        displays.push_back(sfmlDisplay);
    }

    if (args.enableColorLight()) {
        ColorLightDisplay* clDisplay = new ColorLightDisplay(args.colorLightInterface(), dfb);
        displays.push_back(clDisplay);
    }

    if (displays.empty()) {
        std::cerr << "No display enabled. Please use -s or -c to enable at least one display." << std::endl;
        return 1;
    }

    ResourceLocator resourceLocator;
    ScoreboardController scoreboard;
    ScoreboardRenderer renderer(dfb, resourceLocator);

    scoreboard.setHomeTeamName("MAMBAS");
    scoreboard.setAwayTeamName("BREAKERS");
    scoreboard.setClockMode(ClockMode::Running);

    std::cout << "Starting scoreboard loop..." << std::endl;

    sf::Clock clock;

    // Main application loop
    bool running = true;
    while(running) {
        if (sfmlDisplay && !sfmlDisplay->isOpen()) {
            running = false;
        }
        if (!running) break;

        // --- LOGIC ---
        sf::Time elapsed = clock.restart();
        scoreboard.update(elapsed.asSeconds());

        // --- RENDER ---
        renderer.render(scoreboard.getState());

        // --- DISPLAY ---
        dfb.swap();

        for (IDisplay* disp : displays) {
            disp->output();
        }

        // Avoid pegged CPU
        sf::sleep(sf::milliseconds(10));
    }

    for (IDisplay* disp : displays) {
        delete disp;
    }
    displays.clear();

    return 0;
}
