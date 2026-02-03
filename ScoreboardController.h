#pragma once

#include <blend2d.h>
#include <string> // Added missing include
#include "display/DoubleFramebuffer.h"
#include "ResourceLocator.h" // Include ResourceLocator

class ScoreboardController {
public:
    explicit ScoreboardController(DoubleFramebuffer& dfb, const ResourceLocator& resourceLocator);

    void render();

    // Scoreboard state management methods
    void setHomeScore(int score);
    void setAwayScore(int score);
    void setTime(int minutes, int seconds);
    void setHomeShots(int shots);
    void setAwayShots(int shots);
    void setHomePenalty(int index, int seconds, int playerNumber);
    void setAwayPenalty(int index, int seconds, int playerNumber);
    void setCurrentPeriod(int period);
    void setHomeTeamName(const std::string& name);
    void setAwayTeamName(const std::string& name);

private:
    DoubleFramebuffer& dfb;

    // Scoreboard state
    int homeScore = 0;
    int awayScore = 0;
    int timeMinutes = 12;
    int timeSeconds = 34;
    int homeShots = 0;
    int awayShots = 0;

    struct Penalty {
        int secondsRemaining = 0;
        int playerNumber = 0;
    };

    Penalty homePenalties[2];
    Penalty awayPenalties[2];

    int currentPeriod = 1;
    std::string homeTeamName = "HOME"; // New
    std::string awayTeamName = "AWAY"; // New

    BLFontFace fontFace;
    BLFont font;
    BLFont shotsFont; // New font for shots on goal

private: // Added this line to move it to private scope
    const ResourceLocator& _resourceLocator;

    void loadFont(const char* path);
};