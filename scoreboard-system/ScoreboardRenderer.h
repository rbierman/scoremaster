#pragma once

#include <blend2d.h>
#include "display/DoubleFramebuffer.h"
#include "ResourceLocator.h"
#include "ScoreboardState.h"

class ScoreboardRenderer {
public:
    explicit ScoreboardRenderer(DoubleFramebuffer& dfb, const ResourceLocator& resourceLocator);

    void render(const ScoreboardState& state) const;

private:
    DoubleFramebuffer& dfb;
    const ResourceLocator& _resourceLocator;

    BLFontFace fontFace;
    BLFont font;
    BLFont shotsFont;
    BLFont periodFont;
    BLFont penaltyFont;
    BLFont labelFont;
    BLFont teamNameFont;

    BLRgba32 colorWhite{255, 255, 255};
    BLRgba32 colorOrange{255, 170, 51};
    BLRgba32 colorRed{255, 0, 0};

    void loadFont(const char* path);
};
