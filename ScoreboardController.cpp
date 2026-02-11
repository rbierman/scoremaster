#include "ScoreboardController.h"

ScoreboardController::ScoreboardController() {
}

const ScoreboardState& ScoreboardController::getState() const {
    return state;
}

void ScoreboardController::setHomeScore(int score) {
    state.homeScore = score;
}

void ScoreboardController::setAwayScore(int score) {
    state.awayScore = score;
}

void ScoreboardController::setTime(int minutes, int seconds) {
    state.timeMinutes = minutes;
    state.timeSeconds = seconds;
}

void ScoreboardController::setHomeShots(int shots) {
    state.homeShots = shots;
}

void ScoreboardController::setAwayShots(int shots) {
    state.awayShots = shots;
}

void ScoreboardController::setHomePenalty(int index, int seconds, int playerNumber) {
    if (index >= 0 && index < 2) {
        state.homePenalties[index].secondsRemaining = seconds;
        state.homePenalties[index].playerNumber = playerNumber;
    }
}

void ScoreboardController::setAwayPenalty(int index, int seconds, int playerNumber) {
    if (index >= 0 && index < 2) {
        state.awayPenalties[index].secondsRemaining = seconds;
        state.awayPenalties[index].playerNumber = playerNumber;
    }
}

void ScoreboardController::setCurrentPeriod(int period) {
    state.currentPeriod = period;
}

void ScoreboardController::setHomeTeamName(const std::string& name) {
    state.homeTeamName = name;
}

void ScoreboardController::setAwayTeamName(const std::string& name) {
    state.awayTeamName = name;
}
