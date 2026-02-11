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
    labelFont.createFromFace(fontFace, 20.0f); // Even smaller font for labels
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

    // Clear the background
    ctx.clearAll();


    // Set the fill style for the text
    ctx.setFillStyle(BLRgba32(0xFFFFFFFF)); // White

    // Hardcoded positions for 384x160 display
    double penaltyTextY = 150.0; // Baseline Y position for penalties

    BLGlyphBuffer gb; // Initialize glyph buffer once

    // --- Metrics and Positioning ---
    BLFontMetrics teamNameFontMetrics = shotsFont.metrics();
    double teamNameTextY = teamNameFontMetrics.capHeight + 4;

    BLFontMetrics mainFontMetrics = font.metrics();
    double clockTextY = mainFontMetrics.capHeight + 4;

    // Time Metrics (needed for centering team names relative to time border)
    std::stringstream minStringStream;
    minStringStream << std::setfill('0') << std::setw(2) << timeMinutes;
    auto minutes = minStringStream.str();
    std::string colonStr = ":";
    std::stringstream secStringStream;
    secStringStream << std::setfill('0') << std::setw(2) << timeSeconds;
    auto seconds = secStringStream.str();

    BLTextMetrics tmMin, tmColon, tmSec;
    gb.setUtf8Text(minutes.c_str(), minutes.length());
    font.getTextMetrics(gb, tmMin);
    gb.clear();

    gb.setUtf8Text(colonStr.c_str(), colonStr.length());
    font.getTextMetrics(gb, tmColon);
    gb.clear();

    gb.setUtf8Text(seconds.c_str(), seconds.length());
    font.getTextMetrics(gb, tmSec);
    gb.clear();

    double spacingAdjustment = 10.0;
    double timeWidth = tmMin.advance.x + tmColon.advance.x + tmSec.advance.x - (2 * spacingAdjustment);
    double startX = (w / 2.0) - (timeWidth / 2.0);
    int timePadding = 2;
    int timeRectX = (int)startX - timePadding;

    // --- Team Names ---
    ctx.setFillStyle(BLRgba32(0xFFFFFFFF)); // White for team names

    // Home Team Name (Centered in the left area)
    gb.setUtf8Text(homeTeamName.c_str(), homeTeamName.length());
    BLTextMetrics tmHomeName;
    shotsFont.getTextMetrics(gb, tmHomeName);
    gb.clear();
    double homeTeamNameWidth = tmHomeName.advance.x;
    double homeTeamNameX = (timeRectX / 2.0) - (homeTeamNameWidth / 2.0);
    ctx.fillUtf8Text(BLPoint(homeTeamNameX, teamNameTextY), shotsFont, homeTeamName.c_str());

    // Away Team Name (Centered in the right area)
    gb.setUtf8Text(awayTeamName.c_str(), awayTeamName.length());
    BLTextMetrics tmAwayName;
    shotsFont.getTextMetrics(gb, tmAwayName);
    gb.clear();

    double awayTeamNameWidth = tmAwayName.advance.x;
    int timeRectW = (int)timeWidth + (2 * timePadding);
    double awayTeamNameX = (timeRectX + timeRectW + w) / 2.0 - (awayTeamNameWidth / 2.0);
    ctx.fillUtf8Text(BLPoint(awayTeamNameX, teamNameTextY), shotsFont, awayTeamName.c_str());

    // Draw the time (centered)
    double currentX = startX;

    ctx.setFillStyle(BLRgba32(0xFFFFAA00)); // Orange (ABGR)

    gb.setUtf8Text(minutes.c_str(), minutes.length());
    ctx.fillUtf8Text(BLPoint(currentX, clockTextY), font, minutes.c_str());
    currentX += tmMin.advance.x - spacingAdjustment;

    gb.setUtf8Text(colonStr.c_str(), colonStr.length());
    ctx.fillUtf8Text(BLPoint(currentX, clockTextY), font, colonStr.c_str());
    currentX += tmColon.advance.x - spacingAdjustment;

    gb.setUtf8Text(seconds.c_str(), seconds.length());
    ctx.fillUtf8Text(BLPoint(currentX, clockTextY), font, seconds.c_str());

    // Draw 1px white border around the time
    ctx.setStrokeStyle(BLRgba32(0xFFFFFFFF));
    ctx.setStrokeWidth(2.0);
    int timeRectY = -1;
    int timeRectH = (int)clockTextY + (3 * timePadding);
    ctx.strokeRect(timeRectX, timeRectY, timeRectW, timeRectH);

    gb.clear();
    // Calculate Y position for the line below team names
    // --- Scores ---
    ctx.setFillStyle(BLRgba32(0xFF0000FF)); // Red for scores (ABGR)

    // Draw the home score (Centered in the left area)
    BLFontMetrics fontMetrics = font.metrics();
    double mainTextY = 28 + fontMetrics.ascent - 6;
    std::string homeScoreStr = std::to_string(homeScore);

    BLTextMetrics tmHomeScore{};
    gb.setUtf8Text(homeScoreStr.c_str(), homeScoreStr.length());
    font.getTextMetrics(gb, tmHomeScore);
    gb.clear();
    double homeScoreWidth = tmHomeScore.advance.x;
    double homeScoreX = (timeRectX / 2.0) - (homeScoreWidth / 2.0);
    ctx.fillUtf8Text(BLPoint(homeScoreX, mainTextY), font, homeScoreStr.c_str());

    // Draw the away score (Centered in the right area)
    std::string awayScoreStr = std::to_string(awayScore);
    BLTextMetrics tmAwayScore{}; // For away score, using main font
    gb.setUtf8Text(awayScoreStr.c_str(), awayScoreStr.length());
    font.getTextMetrics(gb, tmAwayScore);
    gb.clear();
    double awayScoreWidth = tmAwayScore.advance.x; // Use advance.x for width
    double awayScoreX = (timeRectX + timeRectW + w) / 2.0 - (awayScoreWidth / 2.0);
    ctx.fillUtf8Text(BLPoint(awayScoreX , mainTextY), font, awayScoreStr.c_str());

    // Period label and number (centered under the time, at score height)
    std::string periodLabel = "PERIOD:";
    std::string periodNum = std::to_string(currentPeriod);

    BLTextMetrics tmPeriodLabel{}, tmPeriodNum{};
    gb.setUtf8Text(periodLabel.c_str(), periodLabel.length());
    shotsFont.getTextMetrics(gb, tmPeriodLabel);
    gb.clear();

    gb.setUtf8Text(periodNum.c_str(), periodNum.length());
    shotsFont.getTextMetrics(gb, tmPeriodNum);
    gb.clear();

    double totalPeriodWidth = tmPeriodLabel.advance.x + tmPeriodNum.advance.x;
    double periodX = timeRectX + (timeRectW / 2.0) - (totalPeriodWidth / 2.0);

    ctx.setFillStyle(BLRgba32(0xFFFFFFFF)); // White for "PERIOD:"
    ctx.fillUtf8Text(BLPoint(periodX, mainTextY), shotsFont, periodLabel.c_str());
    
    // Underline "PERIOD" (part of the label)
    ctx.setStrokeStyle(BLRgba32(0xFFFFFFFF));
    ctx.setStrokeWidth(1.0);
    // Only underline the "PERIOD" part, excluding the colon if possible, 
    // but for simplicity we'll underline the whole label "PERIOD:"
    ctx.strokeLine(periodX, mainTextY + 2, periodX + tmPeriodLabel.advance.x, mainTextY + 2);

    ctx.setFillStyle(BLRgba32(0xFF0000FF)); // Red for period number
    ctx.fillUtf8Text(BLPoint(periodX + tmPeriodLabel.advance.x, mainTextY), shotsFont, periodNum.c_str());

    // --- Penalties ---
    ctx.setFillStyle(BLRgba32(0xFFFFAA00)); // Orange for penalties

    auto formatTime = [](const int totalSeconds) {
        const int m = totalSeconds / 60;
        const int s = totalSeconds % 60;
        std::stringstream ss;
        ss << m << ":" << std::setfill('0') << std::setw(2) << s;
        return ss.str();
    };

    BLFontMetrics labelFontMetrics = labelFont.metrics();
    double penaltyLabelY = mainTextY + 6 + labelFontMetrics.ascent;
    double penaltyRow1Y = penaltyLabelY + teamNameFontMetrics.ascent +2 ;
    double penaltyRow2Y = penaltyRow1Y + teamNameFontMetrics.ascent;

    // Penalty Column Offsets
    double plyrOffset = 5.0;
    double timeOffset = 65.0;

    // Home Penalty Labels
    ctx.setFillStyle(BLRgba32(0xFFFFFFFF)); // White for labels
    ctx.fillUtf8Text(BLPoint(plyrOffset, penaltyLabelY), labelFont, "PLYR");
    
    BLTextMetrics tmPlyrLabel;
    gb.setUtf8Text("PLYR", 4);
    labelFont.getTextMetrics(gb, tmPlyrLabel);
    gb.clear();
    ctx.setStrokeStyle(BLRgba32(0xFFFFFFFF));
    ctx.setStrokeWidth(1.0);
    ctx.strokeLine(plyrOffset, penaltyLabelY + 2, plyrOffset + tmPlyrLabel.advance.x, penaltyLabelY + 2);

    ctx.fillUtf8Text(BLPoint(timeOffset, penaltyLabelY), labelFont, "PENALTY");
    BLTextMetrics tmPenaltyLabel;
    gb.setUtf8Text("PENALTY", 7);
    labelFont.getTextMetrics(gb, tmPenaltyLabel);
    gb.clear();
    ctx.strokeLine(timeOffset, penaltyLabelY + 2, timeOffset + tmPenaltyLabel.advance.x, penaltyLabelY + 2);

    // Away Penalty Labels
    double awayPenaltyX = w - tmPenaltyLabel.advance.x - 5.0;
    double awayPlyrX = awayPenaltyX - 60.0;
    double awayTimeX = awayPenaltyX;

    ctx.fillUtf8Text(BLPoint(awayPlyrX, penaltyLabelY), labelFont, "PLYR");
    ctx.strokeLine(awayPlyrX, penaltyLabelY + 2, awayPlyrX + tmPlyrLabel.advance.x, penaltyLabelY + 2);

    ctx.fillUtf8Text(BLPoint(awayPenaltyX, penaltyLabelY), labelFont, "PENALTY");
    ctx.strokeLine(awayPenaltyX, penaltyLabelY + 2, awayPenaltyX + tmPenaltyLabel.advance.x, penaltyLabelY + 2);

    // Home Penalties
    int hRow = 0;
    for (auto &[secondsRemaining, playerNumber] : homePenalties) {
        if (secondsRemaining > 0) {
            double yPos = (hRow == 0) ? penaltyRow1Y : penaltyRow2Y;
            ctx.setFillStyle(BLRgba32(0xFFFFAA00)); // Orange for player number
            ctx.fillUtf8Text(BLPoint(plyrOffset + 5, yPos), shotsFont, std::to_string(playerNumber).c_str());
            ctx.setFillStyle(BLRgba32(0xFF0000FF)); // Red for penalty time
            ctx.fillUtf8Text(BLPoint(timeOffset, yPos), shotsFont, formatTime(secondsRemaining).c_str());
            hRow++;
        }
    }

    // Away Penalties
    int aRow = 0;
    for (auto &[secondsRemaining, playerNumber] : awayPenalties) {
        if (secondsRemaining > 0) {
            double yPos = (aRow == 0) ? penaltyRow1Y : penaltyRow2Y;
            ctx.setFillStyle(BLRgba32(0xFFFFAA00)); // Orange for player number
            ctx.fillUtf8Text(BLPoint(awayPlyrX + 5, yPos), shotsFont, std::to_string(playerNumber).c_str());
            ctx.setFillStyle(BLRgba32(0xFF0000FF)); // Red for penalty time 
            ctx.fillUtf8Text(BLPoint(awayTimeX, yPos), shotsFont, formatTime(secondsRemaining).c_str());
            aRow++;
        }
    }

    // --- Shots on Goal ---
    ctx.setFillStyle(BLRgba32(0xFFCCCCCC)); // Lighter gray for SOG text
    double sogLabelY = 150; // Bottom of the screen
    double sogValueY = sogLabelY - labelFontMetrics.ascent - 2; // Above the label

    // Centered "Shots on goal" label
    std::string sogLabelStr = "Shots on goal";
    BLTextMetrics tmSogLabel;
    gb.setUtf8Text(sogLabelStr.c_str(), sogLabelStr.length());
    labelFont.getTextMetrics(gb, tmSogLabel);
    gb.clear();
    double sogLabelX = w / 2.0 - tmSogLabel.advance.x / 2.0;
    ctx.fillUtf8Text(BLPoint(sogLabelX, sogLabelY), labelFont, sogLabelStr.c_str());

    // Underline "Shots on goal"
    ctx.setStrokeStyle(BLRgba32(0xFFFFFFFF));
    ctx.setStrokeWidth(1.0);
    ctx.strokeLine(sogLabelX, sogLabelY + 2, sogLabelX + tmSogLabel.advance.x, sogLabelY + 2);

    ctx.setFillStyle(BLRgba32(0xFFFFAA00)); // Orange for SOG values

    // Home SOG value (closer to center)
    std::string homeShotsStr = std::to_string(homeShots);
    BLTextMetrics tmHomeShots;
    gb.setUtf8Text(homeShotsStr.c_str(), homeShotsStr.length());
    shotsFont.getTextMetrics(gb, tmHomeShots);
    gb.clear();
    ctx.fillUtf8Text(BLPoint(w / 2.0 - 25.0 - tmHomeShots.advance.x, sogValueY), shotsFont, homeShotsStr.c_str());

    // Away SOG value (closer to center)
    std::string awayShotsStr = std::to_string(awayShots);
    BLTextMetrics tmAwayShots;
    gb.setUtf8Text(awayShotsStr.c_str(), awayShotsStr.length());
    shotsFont.getTextMetrics(gb, tmAwayShots);
    gb.clear();
    ctx.fillUtf8Text(BLPoint(w / 2.0 + 25.0, sogValueY), shotsFont, awayShotsStr.c_str());

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
