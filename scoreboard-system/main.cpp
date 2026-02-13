#include <iostream>
#include <vector>
#include <string>
#include <SFML/System/Sleep.hpp>
#include <chrono>

#include "display/DoubleFramebuffer.h"
#include "display/SFMLDisplay.h"
#include "display/ColorLightDisplay.h"
#include "ScoreboardController.h"
#include "ScoreboardRenderer.h"
#include "KeyboardSimulator.h"
#include "NetworkManager.h"
#include "WebSocketManager.h"
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
    
    WebSocketManager* wsPtr = nullptr;
    
    ScoreboardController scoreboard([&wsPtr](const ScoreboardState& state) {
        if (wsPtr) wsPtr->broadcastState(state);
    });

    WebSocketManager ws(9000, scoreboard);
    wsPtr = &ws;
    ws.start();

    ScoreboardRenderer renderer(dfb, resourceLocator);
    KeyboardSimulator simulator(scoreboard);
    NetworkManager network(8080);
    network.start();

    // Default state: Game mode, clock stopped
    scoreboard.setClockMode(ClockMode::Game);
    if (scoreboard.getState().isClockRunning) {
        scoreboard.toggleClock();
    }

    std::cout << "Starting scoreboard loop..." << std::endl;
    simulator.printInstructions();

    // Main application loop
    bool running = true;
    while(running) {
        if (sfmlDisplay) {
            if (!sfmlDisplay->isOpen()) {
                running = false;
            } else {
                simulator.handleInput(sfmlDisplay->getWindow());
            }
        }
        
        if (!running) break;

        // --- LOGIC ---
        scoreboard.update();

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
