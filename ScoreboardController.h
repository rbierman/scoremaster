#pragma once

#include <string>
#include <chrono>
#include "ScoreboardState.h"

class ScoreboardController {
public:
    ScoreboardController();

    const ScoreboardState& getState() const;

    void update(); // Internal timing
    void update(double deltaTime); // External timing (optional/legacy)

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
    void setClockMode(ClockMode mode);
    void toggleClock();
    void addHomeScore(int delta = 1);
    void addAwayScore(int delta = 1);
    void addHomeShots(int delta = 1);
    void addAwayShots(int delta = 1);
    void addHomePenalty(int seconds, int playerNumber);
    void addAwayPenalty(int seconds, int playerNumber);
    void nextPeriod();
    void resetGame();

private:
    ScoreboardState state;

    double gameTimeRemaining = 0.0;
    std::chrono::steady_clock::time_point lastUpdateTime;
};
