#include "ScoreboardController.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <blend2d.h>
#include <string> // Ensure string is included for std::string functions

ScoreboardController::ScoreboardController(DoubleFramebuffer& dfb, const ResourceLocator& resourceLocator)
    : dfb(dfb), _resourceLocator(resourceLocator) {
    loadFont((_resourceLocator.getFontsDirPath() + "/digital-7 (mono).ttf").c_str());
}

void ScoreboardController::loadFont(const char* path) {
    BLResult err = fontFace.createFromFile(path);

    font.createFromFace(fontFace, 70.0f); // Main score/time font size
    shotsFont.createFromFace(fontFace, 28.0f); // Smaller font for SOG and penalties
}


void ScoreboardController::render() {
    const int w = dfb.getWidth();
    const int h = dfb.getHeight();

    // Create a BLImage that references our pixel buffer.
    BLImage image;
    BLResult err = image.createFromData(
        w, h, BL_FORMAT_PRGB32,
        dfb.getBackData(),          // Raw pixel data
        w * 4,                      // Stride in bytes
        BL_DATA_ACCESS_RW           // Read/write access
    );
    if (err) {
        std::cerr << "Failed to create Blend2D image from data: " << err << std::endl;
        return;
    }

    // Create a rendering context
    BLContext ctx(image);

    // --- Drawing starts here ---

    // Clear the background to a dark blue
    ctx.clearAll();
    // ctx.fillAll(fillAllBLRgba32(0xFF000000));

    // Draw border
    ctx.setStrokeStyle(BLRgba32(0xFFFF0000)); // Bright blue (ABGR)
    ctx.setStrokeWidth(3.0); // 3-pixel wide border
    ctx.strokeRect(0, 0, w, h); // Rectangle covering the whole image

    // Set the fill style for the text
    ctx.setFillStyle(BLRgba32(0xFFFFFFFF)); // White

    // Hardcoded positions for 384x160 display
    double penaltyTextY = 150.0; // Baseline Y position for penalties

    BLGlyphBuffer gb; // Initialize glyph buffer once

    // --- Team Names ---
    ctx.setFillStyle(BLRgba32(0xFFFFFFFF)); // White for team names

    // Calculate teamNameTextY based on font metrics (ascent only)
    BLFontMetrics teamNameFontMetrics = shotsFont.metrics(); // Assuming shotsFont for team names
    double teamNameTextY = teamNameFontMetrics.capHeight + 4;  // Using ascent directly

    // Home Team Name
    ctx.fillUtf8Text(BLPoint(2, teamNameTextY), shotsFont, homeTeamName.c_str());

    // Away Team Name (right-aligned)
    gb.setUtf8Text(awayTeamName.c_str(), awayTeamName.length());
    BLTextMetrics tmAwayName; // Use a distinct variable name
    shotsFont.getTextMetrics(gb, tmAwayName);
    gb.clear();

    double awayTeamNameWidth = tmAwayName.advance.x; // Use advance.x for width
    ctx.fillUtf8Text(BLPoint(w - awayTeamNameWidth - 2, teamNameTextY), shotsFont, awayTeamName.c_str());

    // Draw the time (centered)
    // Calculate clockTextY based on font metrics (ascent only)
    BLFontMetrics mainFontMetrics = font.metrics();
    double clockTextY = mainFontMetrics.capHeight + 4; // Using ascent directly

    std::stringstream timeStream;
    timeStream << std::setfill('0') << std::setw(2) << timeMinutes << ":"
               << std::setfill('0') << std::setw(2) << timeSeconds;
    std::string timeStr = timeStream.str();
    BLTextMetrics tmTime{};
    gb.setUtf8Text(timeStr.c_str(), timeStr.length());
    font.getTextMetrics(gb, tmTime);

    double timeWidth = tmTime.advance.x; // Use advance.x for width
    ctx.setFillStyle(BLRgba32(0xFF0000FF)); // Bright red (ABGR)
    ctx.fillGlyphRun(BLPoint(w / 2 - timeWidth / 2, clockTextY), font, gb.glyphRun());

    // Draw 1px white border around the time
    ctx.setStrokeStyle(BLRgba32(0xFFFFFFFF));
    ctx.setStrokeWidth(2.0);
    int timePadding = 2;
    int timeRectX = (w / 2 - timeWidth / 2) - timePadding;
    int timeRectY = -1;
    int timeRectW = timeWidth + (2 * timePadding);
    int timeRectH = clockTextY + (3 * timePadding);
    ctx.strokeRect(timeRectX, timeRectY, timeRectW, timeRectH);

    gb.clear();
    // Calculate Y position for the line below team names
    // --- Scores ---
    // Draw the home score
    BLFontMetrics fontMetrics = font.metrics(); // Assuming shotsFont for team names
    double mainTextY = 28 + fontMetrics.ascent - 6;
    std::string homeScoreStr = std::to_string(homeScore);
    ctx.fillUtf8Text(BLPoint(2, mainTextY), font, homeScoreStr.c_str());

    // Draw the away score
    std::string awayScoreStr = std::to_string(awayScore);
    BLTextMetrics tmAwayScore{}; // For away score, using main font
    gb.setUtf8Text(awayScoreStr.c_str(), awayScoreStr.length());
    font.getTextMetrics(gb, tmAwayScore);
    gb.clear();
    double awayScoreWidth = tmAwayScore.advance.x; // Use advance.x for width
    ctx.fillUtf8Text(BLPoint(w - awayScoreWidth - 4 , mainTextY), font, awayScoreStr.c_str());


    // Draw horizontal line below the score / time
    double lineBelowScoreTimeY = 94;
    ctx.setStrokeStyle(BLRgba32(0xFFFF0000)); // Bright blue (ABGR)
    ctx.setStrokeWidth(1.0); // 1-pixel wide line
    ctx.strokeLine(0, lineBelowScoreTimeY, w, lineBelowScoreTimeY);


    // --- Shots on Goal ---
    double sogTextY = lineBelowScoreTimeY + 2 + teamNameFontMetrics.ascent;
    ctx.setFillStyle(BLRgba32(0xFFCCCCCC)); // Lighter gray for SOG text

    // Home SOG
    std::string homeShotsStr = "SOG:" + std::to_string(homeShots);
    ctx.fillUtf8Text(BLPoint(2, sogTextY), shotsFont, homeShotsStr.c_str()); // 60 for number + padding


    // Period label and number (centered)
    std::string periodStr = "PER:" + std::to_string(currentPeriod);
    BLTextMetrics tmPeriod{};
    gb.setUtf8Text(periodStr.c_str(), periodStr.length());
    shotsFont.getTextMetrics(gb, tmPeriod);
    gb.clear();
    double periodWidth = tmPeriod.advance.x; // Use advance.x for width
    ctx.fillUtf8Text(BLPoint(w / 2 - periodWidth / 2, sogTextY), shotsFont, periodStr.c_str());

    // Away SOG number
    std::string awayShotsStr = "SOG:" + std::to_string(awayShots);
    BLTextMetrics tmAwayShots;
    gb.setUtf8Text(awayShotsStr.c_str(), awayShotsStr.length());
    shotsFont.getTextMetrics(gb, tmAwayShots);
    gb.clear();
    double awayShotsWidth = tmAwayShots.advance.x; // Use advance.x for width
    ctx.fillUtf8Text(BLPoint(w - awayShotsWidth - 2, sogTextY), shotsFont, awayShotsStr.c_str());

    // Draw horizontal line below the score / time
    double lineBelowSOGY = 125;
    ctx.setStrokeStyle(BLRgba32(0xFFFF0000)); // Bright blue (ABGR)
    ctx.setStrokeWidth(1.0); // 1-pixel wide line
    ctx.strokeLine(0, lineBelowSOGY, w, lineBelowSOGY);

    // --- Penalties ---
    ctx.setFillStyle(BLRgba32(0xFFFFAA00)); // Orange for penalties

    auto formatTime = [](const int totalSeconds) {
        const int m = totalSeconds / 60;
        const int s = totalSeconds % 60;
        std::stringstream ss;
        ss << m << ":" << std::setfill('0') << std::setw(2) << s;
        return ss.str();
    };

    // Home Penalties (Left)
    double homePenaltyX = 5.0; // Start with some padding
    for (auto &[secondsRemaining, playerNumber] : homePenalties) {
        if (secondsRemaining > 0) {
             std::string pStr = std::to_string(playerNumber) + " " + formatTime(secondsRemaining);
             ctx.fillUtf8Text(BLPoint(homePenaltyX, penaltyTextY), shotsFont, pStr.c_str());

             // Advance X position
             BLTextMetrics tm{};
             gb.setUtf8Text(pStr.c_str(), pStr.length());
             shotsFont.getTextMetrics(gb, tm);
             gb.clear();
             homePenaltyX += tm.advance.x + 15; // Spacing between penalties
        }
    }

    // Away Penalties (Right)
    double awayPenaltyX = w - 5.0; // Start with some padding
    for (auto &[secondsRemaining, playerNumber] : awayPenalties) {
        if (secondsRemaining > 0) {
             std::string pStr = std::to_string(playerNumber) + " " + formatTime(secondsRemaining);

             BLTextMetrics tm{};
             gb.setUtf8Text(pStr.c_str(), pStr.length());
             shotsFont.getTextMetrics(gb, tm);
             gb.clear();

             ctx.fillUtf8Text(BLPoint(awayPenaltyX - tm.advance.x, penaltyTextY), shotsFont, pStr.c_str());
             awayPenaltyX -= (tm.advance.x + 15); // Spacing between penalties
        }
    }

    ctx.end();
}

void ScoreboardController::setHomeScore(int score) {
    homeScore = score;
}

void ScoreboardController::setAwayScore(int score) {
    awayScore = score;
}

void ScoreboardController::setTime(int minutes, int seconds) {
    timeMinutes = minutes;
    timeSeconds = seconds;
}

void ScoreboardController::setHomeShots(int shots) {
    homeShots = shots;
}

void ScoreboardController::setAwayShots(int shots) {
    awayShots = shots;
}

void ScoreboardController::setHomePenalty(int index, int seconds, int playerNumber) {
    if (index >= 0 && index < 2) {
        homePenalties[index].secondsRemaining = seconds;
        homePenalties[index].playerNumber = playerNumber;
    }
}

void ScoreboardController::setAwayPenalty(int index, int seconds, int playerNumber) {
    if (index >= 0 && index < 2) {
        awayPenalties[index].secondsRemaining = seconds;
        awayPenalties[index].playerNumber = playerNumber;
    }
}

void ScoreboardController::setCurrentPeriod(int period) {
    currentPeriod = period;
}

void ScoreboardController::setHomeTeamName(const std::string& name) {
    homeTeamName = name;
}

void ScoreboardController::setAwayTeamName(const std::string& name) {
    awayTeamName = name;
}
