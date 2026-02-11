#pragma once

#include <string>
#include <vector>

struct Penalty {
    int secondsRemaining = 0;
    int playerNumber = 0;
};

enum class ClockMode {
    Running,
    Stopped,
    Clock
};

struct ScoreboardState {
    int homeScore = 0;
    int awayScore = 0;
    int timeMinutes = 12;
    int timeSeconds = 34;
    int homeShots = 0;
    int awayShots = 0;
    Penalty homePenalties[2];
    Penalty awayPenalties[2];
    int currentPeriod = 1;
    std::string homeTeamName = "HOME";
    std::string awayTeamName = "AWAY";
    ClockMode clockMode = ClockMode::Stopped;
};
