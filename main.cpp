#include <iostream>
#include <unistd.h>
#include <vector>

#include "display/DoubleFramebuffer.h"
#include "display/SFMLDisplay.h"
#include "display/ColorLightDisplay.h"

int main() {
    int w = 64, h = 32;

    // The single source of truth for the scoreboard's state
    DoubleFramebuffer dfb(w, h);

    // A list of displays to send the framebuffer to
    std::vector<IDisplay*> displays;

    // Preview window
    SFMLDisplay sfml(dfb);
    displays.push_back(&sfml);

    // Hardware display
    ColorLightDisplay cl("enx00e04c68012e", dfb);
    displays.push_back(&cl);

    std::cout << "Starting scoreboard loop..." << std::endl;
    while(true) {
        // --- LOGIC ---
        // All drawing happens on the DoubleFramebuffer
        dfb.clearBack();

        // 1. Draw "H" (Home) in Red
        for(int i=2; i<7; i++) {
            dfb.drawPixel(2, i, 255, 50, 50);
            dfb.drawPixel(4, i, 255, 50, 50);
        }
        dfb.drawPixel(3, 4, 255, 50, 50);

        // 2. Draw Home Score (e.g., 5)
        dfb.drawNumber(5, 7, 2, 255, 255, 255);

        // 3. Draw ":" Separator
        dfb.drawPixel(19, 3, 200, 200, 200);
        dfb.drawPixel(19, 5, 200, 200, 200);

        // 4. Draw Visitor Score (e.g., 2)
        dfb.drawNumber(2, 30, 2, 255, 255, 255);

        // 5. Draw "V" (Visitor) in Blue
        for(int i=2; i<5; i++) {
            dfb.drawPixel(35, i, 50, 50, 255);
            dfb.drawPixel(39, i, 50, 50, 255);
        }
        dfb.drawPixel(36, 5, 50, 50, 255);
        dfb.drawPixel(38, 5, 50, 50, 255);
        dfb.drawPixel(37, 6, 50, 50, 255);


        // --- DISPLAY ---
        // Swap buffers to make the drawing visible to the displays
        dfb.swap();

        // Output to all displays
        for (const auto disp : displays) {
            disp->output();
        }

        usleep(33333); // ~30 FPS
    }

    return 0;
}
