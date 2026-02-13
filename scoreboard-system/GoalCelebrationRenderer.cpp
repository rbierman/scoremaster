#include "GoalCelebrationRenderer.h"
#include <iostream>
#include <chrono>

GoalCelebrationRenderer::GoalCelebrationRenderer(DoubleFramebuffer& dfb, const ResourceLocator& resourceLocator, const ScoreboardController& controller)
    : dfb(dfb), _resourceLocator(resourceLocator), controller(controller) {
    
    BLResult err = fontFace.createFromFile((_resourceLocator.getFontsDirPath() + "/digital-7 (mono).ttf").c_str());
    if (err) {
        std::cerr << "Failed to load font for GoalCelebrationRenderer" << std::endl;
    }
    titleFont.createFromFace(fontFace, 50.0f);
    playerFont.createFromFace(fontFace, 30.0f);
}

void GoalCelebrationRenderer::render() const {
    const int w = dfb.getWidth();
    const int h = dfb.getHeight();
    const ScoreboardState& state = controller.getState();

    BLImage image;
    image.createFromData(w, h, BL_FORMAT_PRGB32, dfb.getBackData(), w * 4, BL_DATA_ACCESS_RW);
    BLContext ctx(image);

    ctx.clearAll();

    // 1. Render Player Image (Full Height: 160px, Clipped to Circle)
    const auto& imageData = controller.getGoalPlayerImageData();
    if (!imageData.empty()) {
        BLImage playerImg;
        if (playerImg.readFromData(imageData.data(), imageData.size()) == BL_SUCCESS) {
            double targetH = (double)h; // h is 160
            double scale = targetH / playerImg.height();
            double targetW = playerImg.width() * scale;
            
            double imgX = (w - targetW) / 2.0;
            double imgY = 0.0;
            
            // Create circular path
            double centerX = w / 2.0;
            double centerY = h / 2.0;
            double radius = h / 2.0 - 5.0; // Slightly smaller than full height
            
            ctx.save();
            ctx.setFillStyle(colorWhite);
            ctx.fillCircle(BLCircle(centerX, centerY, radius));
            
            ctx.setCompOp(BL_COMP_OP_SRC_IN);
            ctx.blitImage(BLRect(imgX, imgY, targetW, targetH), playerImg);
            ctx.restore();

            // Optional: Draw a thin white border around the circle
            ctx.setStrokeStyle(colorWhite);
            ctx.setStrokeWidth(2.0);
            ctx.strokeCircle(BLCircle(centerX, centerY, radius));
        }
    }

    // 2. Render "GOAL!" text (Blinking at top left and top right)
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    bool showGoal = (ms / 500) % 2 == 0; // 500ms blink rate

    if (showGoal) {
        ctx.setFillStyle(colorRed);
        std::string goalText = "GOAL!";
        
        // Top Left
        ctx.fillUtf8Text(BLPoint(10.0, 40.0), titleFont, goalText.c_str());
        
        // Top Right
        BLGlyphBuffer gb;
        gb.setUtf8Text(goalText.c_str(), goalText.length());
        BLTextMetrics tm;
        titleFont.getTextMetrics(gb, tm);
        ctx.fillUtf8Text(BLPoint(w - tm.advance.x - 10.0, 40.0), titleFont, goalText.c_str());
    }

    // 3. Render Player Name and Number
    double padding = 10.0;
    if (state.goalEvent.playerNumber > 0) {
        std::string playerNum = "#" + std::to_string(state.goalEvent.playerNumber);
        ctx.setFillStyle(colorOrange);
        ctx.fillUtf8Text(BLPoint(padding, h - 10.0), playerFont, playerNum.c_str());
    }

    if (!state.goalEvent.playerName.empty()) {
        std::string playerName = state.goalEvent.playerName;
        BLGlyphBuffer gb;
        gb.setUtf8Text(playerName.c_str(), playerName.length());
        BLTextMetrics tmName;
        playerFont.getTextMetrics(gb, tmName);
        
        ctx.setFillStyle(colorWhite);
        ctx.fillUtf8Text(BLPoint(w - tmName.advance.x - padding, h - 10.0), playerFont, playerName.c_str());
    }

    ctx.end();
}
