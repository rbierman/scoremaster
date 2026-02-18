#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>

#include "display/DoubleFramebuffer.h"
#include "display/ColorLightDisplay.h"
#include "ScoreboardController.h"
#include "ScoreboardRenderer.h"
#include "GoalCelebrationRenderer.h"
#include "IRenderer.h"
#include "network/NetworkManager.h"
#include "network/WebSocketManager.h"
#include "network/Base64Coder.h"
#include "TeamManager.h"
#include "CommandLineArgs.h"
#include "ResourceLocator.h"

#ifdef ENABLE_SFML
#include "display/SFMLDisplay.h"
#include "KeyboardControl.h"
#endif

int main(int argc, char* argv[]) {

    CommandLineArgs args(argc, argv);

    if (args.showHelp()) {
        args.printHelp(argv[0]);
        return 0;
    }

    int w = 384, h = 160;

    DoubleFramebuffer dfb(w, h);
    std::vector<IDisplay*> displays;

#ifdef ENABLE_SFML
    SFMLDisplay* sfmlDisplay = nullptr;
    if (args.enableSFML()) {
        sfmlDisplay = new SFMLDisplay(dfb);
        displays.push_back(sfmlDisplay);
    }
#endif

    if (args.enableColorLight()) {
        ColorLightDisplay* clDisplay = new ColorLightDisplay(args.colorLightInterface(), dfb);
        displays.push_back(clDisplay);
    }

    if (displays.empty()) {
        std::cerr << "No display enabled. Please use -c to enable ColorLight (or -s for SFML if supported)." << std::endl;
        return 1;
    }

    ResourceLocator resourceLocator;
    TeamManager teamManager(resourceLocator.getDataDirPath());
    Base64Coder base64Coder;
    
    WebSocketManager* wsPtr = nullptr;
    
    ScoreboardController scoreboard([&wsPtr](const ScoreboardState& state) {
        if (wsPtr) wsPtr->broadcastState(state);
    });

    WebSocketManager ws(9000, scoreboard, teamManager, base64Coder);
    wsPtr = &ws;
    ws.start();

    ScoreboardRenderer scoreboardRenderer(dfb, resourceLocator, scoreboard.getState());
    GoalCelebrationRenderer goalRenderer(dfb, resourceLocator, scoreboard);

#ifdef ENABLE_SFML
    KeyboardControl simulator(scoreboard);
#endif

    NetworkManager network(9000);
    network.start();

    // Default state: Game mode, clock stopped
    scoreboard.setClockMode(ClockMode::Game);
    if (scoreboard.getState().isClockRunning) {
        scoreboard.toggleClock();
    }

    std::cout << "Starting scoreboard loop..." << std::endl;
    
#ifdef ENABLE_SFML
    simulator.printInstructions();
#endif

    // Main application loop
    bool running = true;
    while(running) {
#ifdef ENABLE_SFML
        if (sfmlDisplay) {
            if (!sfmlDisplay->isOpen()) {
                running = false;
            } else {
                simulator.handleInput(sfmlDisplay->getWindow());
            }
        }
#endif
        
        if (!running) break;

        // --- LOGIC ---
        scoreboard.update();

        // --- RENDER ---
        if (scoreboard.getState().goalEvent.active) {
            goalRenderer.render();
        } else {
            scoreboardRenderer.render();
        }

        // --- DISPLAY ---
        dfb.swap();

        for (IDisplay* disp : displays) {
            disp->output();
        }

        // Avoid pegged CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for (IDisplay* disp : displays) {
        delete disp;
    }
    displays.clear();

    return 0;
}
