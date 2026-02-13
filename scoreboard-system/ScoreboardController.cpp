#include "ScoreboardController.h"
#include <chrono>
#include <ctime>
#include <cmath>

ScoreboardController::ScoreboardController(StateChangeListener listener) : onStateChanged(listener) {
    // Initialize high-res timer from state
    gameTimeRemaining = state.timeMinutes * 60.0 + state.timeSeconds;
    lastUpdateTime = std::chrono::steady_clock::now();
}

const ScoreboardState& ScoreboardController::getState() const {
    return state;
}

void ScoreboardController::notifyStateChanged() {
    if (onStateChanged) {
        onStateChanged(state);
    }
}

void ScoreboardController::update() {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - lastUpdateTime;
    lastUpdateTime = now;
    update(diff.count());
}

void ScoreboardController::update(double deltaTime) {
    if (state.clockMode == ClockMode::Running || state.clockMode == ClockMode::Intermission) {
        int oldSeconds = static_cast<int>(std::ceil(gameTimeRemaining));
        gameTimeRemaining -= deltaTime;
        
        if (gameTimeRemaining <= 0) {
            if (state.clockMode == ClockMode::Intermission) {
                setTime(0, 0);
                setClockMode(ClockMode::Stopped);
            } else {
                resetGame();
            }
            return;
        }

        int newSeconds = static_cast<int>(std::ceil(gameTimeRemaining));

        if (newSeconds < oldSeconds && state.clockMode == ClockMode::Running) {
            int secondsPassed = oldSeconds - newSeconds;
            for (int i = 0; i < 2; ++i) {
                if (state.homePenalties[i].secondsRemaining > 0) {
                    state.homePenalties[i].secondsRemaining -= secondsPassed;
                    if (state.homePenalties[i].secondsRemaining <= 0) {
                        state.homePenalties[i].secondsRemaining = 0;
                        state.homePenalties[i].playerNumber = 0;
                    }
                }
                if (state.awayPenalties[i].secondsRemaining > 0) {
                    state.awayPenalties[i].secondsRemaining -= secondsPassed;
                    if (state.awayPenalties[i].secondsRemaining <= 0) {
                        state.awayPenalties[i].secondsRemaining = 0;
                        state.awayPenalties[i].playerNumber = 0;
                    }
                }
            }
        }

        state.timeMinutes = newSeconds / 60;
        state.timeSeconds = newSeconds % 60;
        
        // We might want to notify every second if needed, but for now we'll notify in update loop if state changes significantly or periodically
        // For efficiency, maybe only notify when time changes (every second)
        if (newSeconds != oldSeconds) {
            notifyStateChanged();
        }

    } else if (state.clockMode == ClockMode::Clock) {
        int oldSec = state.timeSeconds;
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_c);
        state.timeMinutes = now_tm->tm_hour;
        state.timeSeconds = now_tm->tm_min; // Wait, wait, this should be minutes and seconds for display usually, but here it's hour/min
        if (state.timeSeconds != oldSec) {
            notifyStateChanged();
        }
    }
}

void ScoreboardController::setClockMode(ClockMode mode) {
    if (state.clockMode != mode) {
        state.clockMode = mode;
        notifyStateChanged();
    }
}

void ScoreboardController::toggleClock() {
    if (state.clockMode == ClockMode::Running || state.clockMode == ClockMode::Intermission) {
        // If it was intermission, we'll just switch to Stopped but we might want to remember we were in intermission.
        // For now, let's just make it Stopped.
        state.clockMode = ClockMode::Stopped;
    } else {
        // Default to Running when starting from Stopped/Clock
        state.clockMode = ClockMode::Running;
    }
    notifyStateChanged();
}

void ScoreboardController::addHomeScore(int delta) {
    state.homeScore += delta;
    if (state.homeScore < 0) state.homeScore = 0;
    notifyStateChanged();
}

void ScoreboardController::addAwayScore(int delta) {
    state.awayScore += delta;
    if (state.awayScore < 0) state.awayScore = 0;
    notifyStateChanged();
}

void ScoreboardController::addHomeShots(int delta) {
    state.homeShots += delta;
    if (state.homeShots < 0) state.homeShots = 0;
    notifyStateChanged();
}

void ScoreboardController::addAwayShots(int delta) {
    state.awayShots += delta;
    if (state.awayShots < 0) state.awayShots = 0;
    notifyStateChanged();
}

void ScoreboardController::addHomePenalty(int seconds, int playerNumber) {
    int index = (state.homePenalties[0].secondsRemaining <= 0) ? 0 : 1;
    setHomePenalty(index, seconds, playerNumber);
}

void ScoreboardController::addAwayPenalty(int seconds, int playerNumber) {
    int index = (state.awayPenalties[0].secondsRemaining <= 0) ? 0 : 1;
    setAwayPenalty(index, seconds, playerNumber);
}

void ScoreboardController::nextPeriod() {
    state.currentPeriod++;
    if (state.currentPeriod > 3) state.currentPeriod = 1;
    notifyStateChanged();
}

void ScoreboardController::resetGame() {
    state.clockMode = ClockMode::Stopped;
    gameTimeRemaining = 20 * 60.0; 
    state.timeMinutes = 20;
    state.timeSeconds = 0;

    state.homeScore = 0;
    state.awayScore = 0;
    state.homeShots = 0;
    state.awayShots = 0;
    state.currentPeriod = 1;

    for (int i = 0; i < 2; ++i) {
        state.homePenalties[i].secondsRemaining = 0;
        state.homePenalties[i].playerNumber = 0;
        state.awayPenalties[i].secondsRemaining = 0;
        state.awayPenalties[i].playerNumber = 0;
    }
    notifyStateChanged();
}

void ScoreboardController::setHomeScore(int score) {
    state.homeScore = score;
    notifyStateChanged();
}

void ScoreboardController::setAwayScore(int score) {
    state.awayScore = score;
    notifyStateChanged();
}

void ScoreboardController::setTime(int minutes, int seconds) {
    state.timeMinutes = minutes;
    state.timeSeconds = seconds;
    gameTimeRemaining = minutes * 60.0 + seconds;
    notifyStateChanged();
}

void ScoreboardController::setHomeShots(int shots) {
    state.homeShots = shots;
    notifyStateChanged();
}

void ScoreboardController::setAwayShots(int shots) {
    state.awayShots = shots;
    notifyStateChanged();
}

void ScoreboardController::setHomePenalty(int index, int seconds, int playerNumber) {
    if (index >= 0 && index < 2) {
        state.homePenalties[index].secondsRemaining = seconds;
        state.homePenalties[index].playerNumber = playerNumber;
        notifyStateChanged();
    }
}

void ScoreboardController::setAwayPenalty(int index, int seconds, int playerNumber) {
    if (index >= 0 && index < 2) {
        state.awayPenalties[index].secondsRemaining = seconds;
        state.awayPenalties[index].playerNumber = playerNumber;
        notifyStateChanged();
    }
}

void ScoreboardController::setCurrentPeriod(int period) {
    state.currentPeriod = period;
    notifyStateChanged();
}

void ScoreboardController::setHomeTeamName(const std::string& name) {
    state.homeTeamName = name;
    notifyStateChanged();
}

void ScoreboardController::setAwayTeamName(const std::string& name) {
    state.awayTeamName = name;
    notifyStateChanged();
}
