#include <iostream>
#include <unistd.h>
#include <vector>
#include <random>
#include <string>

#include "display/DoubleFramebuffer.h"
#include "display/SFMLDisplay.h"
#include "display/ColorLightDisplay.h"
#include "ScoreboardController.h"
#include "CommandLineArgs.h" // Include the new header
#include "ResourceLocator.h" // Include ResourceLocator

int main(int argc, char* argv[]) {

    CommandLineArgs args(argc, argv); // Create an instance of the new class

    if (args.showHelp()) { // Check if help was requested
        args.printHelp(argv[0]);
        return 0;
    }

    int w = 384, h = 160;

    DoubleFramebuffer dfb(w, h);
    std::vector<IDisplay*> displays; // Use pointers for polymorphic behavior and dynamic allocation

    SFMLDisplay* sfmlDisplay = nullptr;
    if (args.enableSFML()) { // Use getter
        sfmlDisplay = new SFMLDisplay(dfb);
        displays.push_back(sfmlDisplay);
    }

    if (args.enableColorLight()) { // Use getter
        ColorLightDisplay* clDisplay = new ColorLightDisplay(args.colorLightInterface(), dfb); // Use getter
        displays.push_back(clDisplay);
    }

    if (displays.empty()) {
        std::cerr << "No display enabled. Please use -s or -c to enable at least one display." << std::endl;
        return 1;
    }

    ResourceLocator resourceLocator; // Create ResourceLocator instance
    ScoreboardController scoreboard(dfb, resourceLocator); // Pass ResourceLocator to constructor
    scoreboard.setHomeTeamName("MAMBAS"); // Set initial home team name
    scoreboard.setAwayTeamName("BREAKERS"); // Set initial away team name

    std::cout << "Starting scoreboard loop..." << std::endl;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distribScore(0, 9); // For scores
    std::uniform_int_distribution<> distribShots(0, 50); // For shots
    std::uniform_int_distribution<> distribPenalty(0, 5); // For penalty minutes (0-5 min)
    std::uniform_int_distribution<> distribPeriod(1, 3); // For period (1-3)


    int currentHomeScore = 22;
    int currentAwayScore = 21;
    int currentMinutes = 1;
    int currentSeconds = 0;
    int currentHomeShots = 0;
    int currentAwayShots = 0;
    int homePenaltySeconds[2] = {80, 90};
    int homePenaltyPlayer[2] = {88, 97};
    int awayPenaltySeconds[2] = {60, 40};
    int awayPenaltyPlayer[2] = {13, 22};
    int currentPeriod = 1;


    // Main application loop
    bool running = true;
    while(running) {
        // If SFML display is enabled, its window state controls the loop
        if (sfmlDisplay && !sfmlDisplay->isOpen()) {
            running = false;
        }
        if (!running) break; // Exit if SFML window closed

        // --- LOGIC ---
        if (currentSeconds == 0) {
            currentSeconds = 59;
            currentMinutes--;
            if (currentMinutes < 0) {
                currentMinutes = 20;
                currentHomeScore = distribScore(gen);
                currentAwayScore = distribScore(gen);
                currentHomeShots = distribShots(gen);
                currentAwayShots = distribShots(gen);
                
                homePenaltySeconds[0] = distribPenalty(gen) * 60;
                homePenaltyPlayer[0] = std::uniform_int_distribution<>(1, 99)(gen);
                homePenaltySeconds[1] = 0;
                homePenaltyPlayer[1] = 0;

                awayPenaltySeconds[0] = distribPenalty(gen) * 60;
                awayPenaltyPlayer[0] = std::uniform_int_distribution<>(1, 99)(gen);
                awayPenaltySeconds[1] = 0;
                awayPenaltyPlayer[1] = 0;

                currentPeriod = distribPeriod(gen);
            }
        } else {
            currentSeconds--;
        }

        scoreboard.setHomeScore(currentHomeScore);
        scoreboard.setAwayScore(currentAwayScore);
        scoreboard.setTime(currentMinutes, currentSeconds);
        scoreboard.setHomeShots(currentHomeShots);
        scoreboard.setAwayShots(currentAwayShots);
        for (int i = 0; i < 2; ++i) {
            scoreboard.setHomePenalty(i, homePenaltySeconds[i], homePenaltyPlayer[i]);
            scoreboard.setAwayPenalty(i, awayPenaltySeconds[i], awayPenaltyPlayer[i]);
        }
        scoreboard.setCurrentPeriod(currentPeriod);

        scoreboard.render();

        // --- DISPLAY ---
        dfb.swap();

        for (IDisplay* disp : displays) {
            disp->output();
        }

        usleep(1000000);
    }

    // Clean up dynamically allocated display objects
    for (IDisplay* disp : displays) {
        delete disp;
    }
    displays.clear();

    return 0;
}