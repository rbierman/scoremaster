#include "ScoreboardController.h"
#include <random>
#include <chrono>
#include <ctime>

ScoreboardController::ScoreboardController() {
    std::random_device rd;
    gen.seed(rd());
    // Initialize high-res timers from default state
    gameTimeRemaining = state.timeMinutes * 60.0 + state.timeSeconds;
    for (int i = 0; i < 2; ++i) {
        homePenaltyRemaining[i] = state.homePenalties[i].secondsRemaining;
        awayPenaltyRemaining[i] = state.awayPenalties[i].secondsRemaining;
    }
}

const ScoreboardState& ScoreboardController::getState() const {
    return state;
}

void ScoreboardController::update(double deltaTime) {
    if (state.clockMode == ClockMode::Running) {
        gameTimeRemaining -= deltaTime;
        if (gameTimeRemaining <= 0) {
            resetGame();
        } else {
            state.timeMinutes = static_cast<int>(gameTimeRemaining) / 60;
            state.timeSeconds = static_cast<int>(gameTimeRemaining) % 60;
        }

        for (int i = 0; i < 2; ++i) {
            if (homePenaltyRemaining[i] > 0) {
                homePenaltyRemaining[i] -= deltaTime;
                if (homePenaltyRemaining[i] < 0) homePenaltyRemaining[i] = 0;
                state.homePenalties[i].secondsRemaining = static_cast<int>(homePenaltyRemaining[i]);
            }
            if (awayPenaltyRemaining[i] > 0) {
                awayPenaltyRemaining[i] -= deltaTime;
                if (awayPenaltyRemaining[i] < 0) awayPenaltyRemaining[i] = 0;
                state.awayPenalties[i].secondsRemaining = static_cast<int>(awayPenaltyRemaining[i]);
            }
        }
    } else if (state.clockMode == ClockMode::Clock) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_c);
        state.timeMinutes = now_tm->tm_hour;
        state.timeSeconds = now_tm->tm_min;
        // In clock mode, maybe we want HH:MM? The prompt says "show the current time".
        // If we use timeMinutes for hour and timeSeconds for minute, it fits the UI.
    }
    // If Stopped, do nothing.
}

void ScoreboardController::setClockMode(ClockMode mode) {
    state.clockMode = mode;
}

void ScoreboardController::resetGame() {
    std::uniform_int_distribution<> distribScore(0, 9);
    std::uniform_int_distribution<> distribShots(0, 50);
    std::uniform_int_distribution<> distribPenalty(0, 5);
    std::uniform_int_distribution<> distribPeriod(1, 3);
    std::uniform_int_distribution<> distribPlayer(1, 99);

    gameTimeRemaining = 20 * 60.0; // 20 minutes
    state.timeMinutes = 20;
    state.timeSeconds = 0;

    state.homeScore = distribScore(gen);
    state.awayScore = distribScore(gen);
    state.homeShots = distribShots(gen);
    state.awayShots = distribShots(gen);
    state.currentPeriod = distribPeriod(gen);

    state.homePenalties[0].secondsRemaining = distribPenalty(gen) * 60;
    state.homePenalties[0].playerNumber = distribPlayer(gen);
    homePenaltyRemaining[0] = state.homePenalties[0].secondsRemaining;

    state.homePenalties[1].secondsRemaining = 0;
    state.homePenalties[1].playerNumber = 0;
    homePenaltyRemaining[1] = 0;

    state.awayPenalties[0].secondsRemaining = distribPenalty(gen) * 60;
    state.awayPenalties[0].playerNumber = distribPlayer(gen);
    awayPenaltyRemaining[0] = state.awayPenalties[0].secondsRemaining;

    state.awayPenalties[1].secondsRemaining = 0;
    state.awayPenalties[1].playerNumber = 0;
    awayPenaltyRemaining[1] = 0;
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
    gameTimeRemaining = minutes * 60.0 + seconds;
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
        homePenaltyRemaining[index] = seconds;
    }
}

void ScoreboardController::setAwayPenalty(int index, int seconds, int playerNumber) {
    if (index >= 0 && index < 2) {
        state.awayPenalties[index].secondsRemaining = seconds;
        state.awayPenalties[index].playerNumber = playerNumber;
        awayPenaltyRemaining[index] = seconds;
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
