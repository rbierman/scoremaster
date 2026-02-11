#pragma once

#include <string>
#include <random>
#include "ScoreboardState.h"

class ScoreboardController {
public:
    ScoreboardController();

    const ScoreboardState& getState() const;

    void update(double deltaTime);

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

private:
    ScoreboardState state;

    double gameTimeRemaining = 0.0;
    double homePenaltyRemaining[2] = {0.0, 0.0};
    double awayPenaltyRemaining[2] = {0.0, 0.0};

    std::mt19937 gen;
    void resetGame();
};
