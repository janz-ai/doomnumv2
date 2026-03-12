#include "doom.h"

// ─────────────────────────────────────────────────────────────
//  RAYCASTER  —  Classic DDA wall rendering
//  One ray per screen column → draws ceiling / wall / floor
// ─────────────────────────────────────────────────────────────

void renderWalls(GameState& g) {
    KDContext* ctx = KDIonContext::sharedContext();
    const Player& p = g.player;

    for (int x = 0; x < SCR_W; x++) {

        // ── 1. Ray direction ───────────────────────────────────
        float cameraX  = 2.0f * (float)x / (float)SCR_W - 1.0f;
        float rayDirX  = p.dirX + p.planeX * cameraX;
        float rayDirY  = p.dirY + p.planeY * cameraX;

        // ── 2. Map cell the player is in ───────────────────────
        int mapX = (int)p.x;
        int mapY = (int)p.y;

        // ── 3. Delta distance (length to next grid line) ───────
        float deltaX = (rayDirX == 0.0f) ? 1e30f : fabsf(1.0f / rayDirX);
        float deltaY = (rayDirY == 0.0f) ? 1e30f : fabsf(1.0f / rayDirY);

        // ── 4. Step direction & initial side distance ──────────
        int   stepX, stepY;
        float sideX, sideY;

        if (rayDirX < 0.0f) {
            stepX = -1;
            sideX = (p.x - (float)mapX) * deltaX;
        } else {
            stepX =  1;
            sideX = ((float)mapX + 1.0f - p.x) * deltaX;
        }
        if (rayDirY < 0.0f) {
            stepY = -1;
            sideY = (p.y - (float)mapY) * deltaY;
        } else {
            stepY =  1;
            sideY = ((float)mapY + 1.0f - p.y) * deltaY;
        }

        // ── 5. DDA loop ────────────────────────────────────────
        int  side     = 0;
        int  wallType = 0;
        bool hit      = false;

        while (!hit) {
            if (sideX < sideY) {
                sideX += deltaX;
                mapX  += stepX;
                side   = 0;
            } else {
                sideY += deltaY;
                mapY  += stepY;
                side   = 1;
            }
            // Bounds guard
            if (mapX < 0 || mapX >= MAP_W || mapY < 0 || mapY >= MAP_H) {
                wallType = 1; hit = true; break;
            }
            wallType = WORLD_MAP[mapY][mapX];
            if (wallType > 0) hit = true;
        }

        // ── 6. Perpendicular wall distance (fish-eye corrected) ─
        float perpDist;
        if (side == 0)
            perpDist = sideX - deltaX;
        else
            perpDist = sideY - deltaY;
        if (perpDist < 0.001f) perpDist = 0.001f;

        // Store in z-buffer for sprite rendering
        g.zBuffer[x] = perpDist;

        // ── 7. Wall column height on screen ────────────────────
        int lineH = (int)((float)VIEW_H / perpDist);

        int drawTop = VIEW_H / 2 - lineH / 2;
        int drawBot = VIEW_H / 2 + lineH / 2;
        // Clamp
        int renderTop = drawTop < 0       ? 0       : drawTop;
        int renderBot = drawBot >= VIEW_H  ? VIEW_H-1 : drawBot;

        // ── 8. Pick wall colour ────────────────────────────────
        KDColor wallCol;
        if (wallType == 2) {
            wallCol = (side == 0) ? C_W2_NS : C_W2_EW;
        } else {
            wallCol = (side == 0) ? C_W1_NS : C_W1_EW;
        }

        // Darken very distant walls for atmosphere
        // (simple trick: use darker shade if perpDist > threshold)
        if (perpDist > 6.0f) {
            // blend toward black: shift each nibble right
            uint32_t r = (wallCol.red()   * 120) / 256;
            uint32_t gn= (wallCol.green() * 120) / 256;
            uint32_t b = (wallCol.blue()  * 120) / 256;
            wallCol = KDColor::RGB888((uint8_t)r,(uint8_t)gn,(uint8_t)b);
        }

        // ── 9. Draw column ─────────────────────────────────────
        // Ceiling
        if (renderTop > 0) {
            ctx->fillRect(KDRect(x, 0, 1, renderTop), C_CEIL);
        }
        // Wall
        if (renderBot >= renderTop) {
            ctx->fillRect(KDRect(x, renderTop, 1, renderBot - renderTop + 1), wallCol);
        }
        // Floor
        if (renderBot < VIEW_H - 1) {
            // Simple floor shading: lighter near horizon, darker near bottom
            int floorH = VIEW_H - 1 - renderBot;
            // Split into two bands
            int mid = renderBot + floorH / 2;
            ctx->fillRect(KDRect(x, renderBot + 1, 1,
                                 mid - renderBot),
                          KDColor::RGB888(30, 18, 10));
            ctx->fillRect(KDRect(x, mid + 1, 1,
                                 VIEW_H - 1 - mid),
                          KDColor::RGB888(18, 10,  5));
        }
    }
}
