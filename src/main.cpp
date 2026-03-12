// ─────────────────────────────────────────────────────────────
//  DOOM-NW  ·  Entry point & main game loop
//  NumWorks external app (.nwa)
// ─────────────────────────────────────────────────────────────

#include "doom.h"

// Simple millisecond timer via Ion::Timing (if available)
// Falls back to frame counting if not present
static inline void waitForVsync() {
    // On NumWorks, just poll — the screen refresh limits us
    // to ~30 fps naturally on the STM32F730
    Ion::Timing::msleep(16); // target ~60fps, actual ~30 on hardware
}

int main(int, char**) {
    GameState g;

    // ─── Title screen ─────────────────────────────────────────
    renderTitleScreen();

    // Wait for OK to start
    while (true) {
        Ion::Keyboard::State s = Ion::Keyboard::scan();
        if (s.keyDown(Ion::Keyboard::Key::OK)) break;
        if (s.keyDown(Ion::Keyboard::Key::Back)) return 0;
        waitForVsync();
    }

    // Debounce
    Ion::Timing::msleep(200);

    // ─── Main game loop ───────────────────────────────────────
    while (true) {
        // Init / restart
        initGame(g);

        while (!g.gameOver) {
            // 1. Input
            processInput(g);

            // 2. Update AI & timers
            updateGame(g);

            // 3. Render
            renderWalls(g);     // 3D raycasted walls
            renderEnemies(g);   // Billboard enemy sprites
            renderWeapon(g);    // Weapon at bottom of viewport
            renderHUD(g);       // Status bar

            waitForVsync();
        }

        // ─── Game over / win screen ──────────────────────────
        renderGameOver(g);

        // Wait for choice
        while (true) {
            Ion::Keyboard::State s = Ion::Keyboard::scan();
            if (s.keyDown(Ion::Keyboard::Key::OK))   break;    // restart
            if (s.keyDown(Ion::Keyboard::Key::Back))  return 0; // quit
            waitForVsync();
        }
        Ion::Timing::msleep(300); // debounce
    }

    return 0;
}
