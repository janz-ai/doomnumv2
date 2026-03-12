#include "doom.h"

// ─────────────────────────────────────────────────────────────
//  RENDERER  —  Sprites / Weapon / HUD / Screens
// ─────────────────────────────────────────────────────────────

static KDContext* ctx() { return KDIonContext::sharedContext(); }

// ═════════════════════════════════════════════════════════════
//  HELPERS — fill & draw utilities
// ═════════════════════════════════════════════════════════════

static inline void rect(int x, int y, int w, int h, KDColor c) {
    if (w <= 0 || h <= 0) return;
    // Clip to screen
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCR_W) w = SCR_W - x;
    if (y + h > SCR_H) h = SCR_H - y;
    if (w <= 0 || h <= 0) return;
    ctx()->fillRect(KDRect(x, y, w, h), c);
}

static inline void pixel(int x, int y, KDColor c) {
    if (x < 0 || x >= SCR_W || y < 0 || y >= SCR_H) return;
    ctx()->setPixel(KDPoint(x, y), c);
}

// Draw a 1-pixel-wide horizontal line (faster than rect for 1px)
static inline void hline(int x, int y, int w, KDColor c) {
    rect(x, y, w, 1, c);
}
static inline void vline(int x, int y, int h, KDColor c) {
    rect(x, y, 1, h, c);
}

// ─── Big pixel text (5×7 font, scale factor) ─────────────────
// Minimal 5×7 bitmapped font for digits 0-9 and a few letters
// Each char: 5 columns × 7 rows, bit7=top

static const uint8_t FONT5X7[][5] = {
    // 0
    {0x3E,0x51,0x49,0x45,0x3E},
    // 1
    {0x00,0x42,0x7F,0x40,0x00},
    // 2
    {0x42,0x61,0x51,0x49,0x46},
    // 3
    {0x21,0x41,0x45,0x4B,0x31},
    // 4
    {0x18,0x14,0x12,0x7F,0x10},
    // 5
    {0x27,0x45,0x45,0x45,0x39},
    // 6
    {0x3C,0x4A,0x49,0x49,0x30},
    // 7
    {0x01,0x71,0x09,0x05,0x03},
    // 8
    {0x36,0x49,0x49,0x49,0x36},
    // 9
    {0x06,0x49,0x49,0x29,0x1E},
    // A=10
    {0x7E,0x11,0x11,0x11,0x7E},
    // M=11
    {0x7F,0x02,0x0C,0x02,0x7F},
    // O=12
    {0x3E,0x41,0x41,0x41,0x3E},
    // S=13
    {0x46,0x49,0x49,0x49,0x31},
    // C=14
    {0x3E,0x41,0x41,0x41,0x22},
    // K=15
    {0x7F,0x08,0x14,0x22,0x41},
    // I=16
    {0x00,0x41,0x7F,0x41,0x00},
    // L=17
    {0x7F,0x40,0x40,0x40,0x40},
    // E=18
    {0x7F,0x49,0x49,0x49,0x41},
    // slash=19
    {0x20,0x10,0x08,0x04,0x02},
};

static void drawChar(int idx, int x, int y, int scale, KDColor fg, KDColor bg) {
    if (idx < 0 || idx >= (int)(sizeof(FONT5X7)/sizeof(FONT5X7[0]))) return;
    for (int col = 0; col < 5; col++) {
        uint8_t bits = FONT5X7[idx][col];
        for (int row = 0; row < 7; row++) {
            KDColor c = (bits & (1 << row)) ? fg : bg;
            rect(x + col * scale, y + row * scale, scale, scale, c);
        }
    }
}

static void drawDigit(int d, int x, int y, int scale, KDColor fg, KDColor bg) {
    if (d < 0 || d > 9) return;
    drawChar(d, x, y, scale, fg, bg);
}

static void drawNumber(int n, int x, int y, int scale, KDColor fg, KDColor bg, int pad=3) {
    // Right-align in 'pad' digits
    int divisor = 1;
    for (int i = 1; i < pad; i++) divisor *= 10;
    for (int i = 0; i < pad; i++) {
        int digit = (n / divisor) % 10;
        drawDigit(digit, x + i * (5*scale + scale), y, scale, fg, bg);
        divisor /= 10;
    }
}

// ═════════════════════════════════════════════════════════════
//  ENEMY SPRITES
//  Billboard sprites (scale to distance)
// ═════════════════════════════════════════════════════════════

static void drawEnemy(const GameState& g, int idx) {
    const Enemy& e   = g.enemies[idx];
    const Player& p  = g.player;

    if (!e.alive || e.dist < 0.3f) return;

    int sh = e.spriteH;
    int sw = sh * 2 / 3;   // width ratio
    if (sh < 4 || sw < 2) return;

    int sx = e.screenX - sw / 2;
    int sy = VIEW_H / 2 - sh / 2;

    // ─── Pixel-art demon/zombie ───────────────────────────────
    // Columns: head, torso, legs
    // Colour values scale with distance (darken far enemies)
    float brightness = fclamp(1.0f - e.dist * 0.08f, 0.25f, 1.0f);
    auto dim = [&](KDColor base) -> KDColor {
        uint8_t r = (uint8_t)(base.red()   * brightness);
        uint8_t gn= (uint8_t)(base.green() * brightness);
        uint8_t b = (uint8_t)(base.blue()  * brightness);
        return KDColor::RGB888(r, gn, b);
    };

    KDColor skin  = dim(C_ENM_SK);
    KDColor body  = dim(C_ENM_BDY);
    KDColor cloth = dim(C_ENM_CLO);
    KDColor eyes  = (e.state == 2) ? C_RED : dim(C_ENM_EYE);
    KDColor blk   = C_BLACK;

    int hw = sw;          // head width
    int hh = sh * 3/10;  // head height
    int tw = sw;          // torso width
    int th = sh * 4/10;  // torso height
    int lh = sh - hh - th; // legs height

    int headX = sx;
    int headY = sy;
    int torX  = sx;
    int torY  = sy + hh;
    int legX  = sx;
    int legY  = sy + hh + th;

    // Only draw columns that are NOT behind a wall (z-test)
    for (int col = sx; col < sx + sw; col++) {
        if (col < 0 || col >= SCR_W) continue;
        if (g.zBuffer[col] < e.dist) continue; // behind wall

        // Head zone
        for (int row = headY; row < headY + hh; row++) {
            if (row < 0 || row >= VIEW_H) continue;
            // Border = dark, inside = skin
            bool border = (col == headX || col == headX+hw-1 ||
                           row == headY || row == headY+hh-1);
            KDColor c = border ? body : skin;
            // Eyes (upper-middle of head)
            if (hh > 6) {
                int eyeRow = headY + hh * 2/5;
                int eyeL   = headX + hw/4;
                int eyeR   = headX + hw*3/4;
                if (row == eyeRow &&
                    (col == eyeL || col == eyeL+1 ||
                     col == eyeR || col == eyeR+1))
                    c = eyes;
                // Mouth
                if (row == headY + hh*3/4 && col > headX+hw/4 && col < headX+hw*3/4)
                    c = blk;
            }
            pixel(col, row, c);
        }

        // Torso zone
        for (int row = torY; row < torY + th; row++) {
            if (row < 0 || row >= VIEW_H) continue;
            bool border = (col == torX || col == torX+tw-1 ||
                           row == torY || row == torY+th-1);
            KDColor c = border ? body : cloth;
            pixel(col, row, c);
        }

        // Legs zone
        for (int row = legY; row < legY + lh; row++) {
            if (row < 0 || row >= VIEW_H) continue;
            // Two legs: left and right halves
            bool leftLeg  = (col >= legX && col < legX + tw/2 - 1);
            bool rightLeg = (col >= legX + tw/2 + 1 && col < legX + tw);
            if (leftLeg || rightLeg) {
                pixel(col, row, body);
            }
        }
    }

    // Dying: draw blood splat instead
    if (e.state == 3) {
        int bx = sx + sw/2;
        int by = sy + sh - sh/4;
        int br = sh/6;
        rect(bx - br, by - br, br*2, br*2, C_BLOOD);
        // Darker centre
        rect(bx - br/2, by - br/2, br, br, C_DRED);
    }
}

// ─── Sort enemies by distance (bubble sort — small array) ───
static void sortEnemiesByDist(Enemy* enemies, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - 1 - i; j++) {
            if (enemies[j].dist < enemies[j+1].dist) {
                Enemy tmp = enemies[j];
                enemies[j] = enemies[j+1];
                enemies[j+1] = tmp;
            }
        }
    }
}

void renderEnemies(GameState& g) {
    const Player& p = g.player;

    // Compute sprite screen positions
    for (int i = 0; i < g.numEnemies; i++) {
        Enemy& e = g.enemies[i];
        // Vector from player to enemy
        float ex = e.x - p.x;
        float ey = e.y - p.y;
        e.dist = sqrtf(ex*ex + ey*ey);

        // Transform into camera space using inverse of view matrix
        float invDet = 1.0f / (p.planeX * p.dirY - p.dirX * p.planeY);
        float transX = invDet * ( p.dirY * ex - p.dirX * ey);
        float transY = invDet * (-p.planeY * ex + p.planeX * ey);

        if (transY <= 0.01f) {
            e.screenX = -9999; // behind player
            e.spriteH = 0;
            continue;
        }

        e.screenX = (int)((SCR_W / 2) * (1.0f + transX / transY));
        e.spriteH = abs((int)((float)VIEW_H / transY));
    }

    sortEnemiesByDist(g.enemies, g.numEnemies);

    for (int i = 0; i < g.numEnemies; i++) {
        drawEnemy(g, i);
    }
}

// ═════════════════════════════════════════════════════════════
//  WEAPON — pixel-art shotgun at the bottom of the viewport
// ═════════════════════════════════════════════════════════════

void renderWeapon(GameState& g) {
    // Weapon bob when moving
    int bob = (g.frame % 16 < 8) ? 1 : 0;

    // Recoil when shooting
    int recoil = (g.player.flashTimer > 0.0f) ? 6 : 0;

    int ox = SCR_W / 2 - 55;  // weapon left edge
    int oy = HUD_Y - 52 + recoil + bob; // weapon top

    // ─── SHOTGUN PIXEL ART ────────────────────────────────────
    // Barrel (long horizontal tube)
    rect(ox + 10, oy + 10, 90, 7, C_GUN_M);   // barrel body
    rect(ox + 10, oy + 10, 90, 2, C_GUN_H);   // barrel top highlight
    rect(ox + 10, oy + 15, 90, 2, C_GUN_D);   // barrel bottom shadow
    // Barrel tip
    rect(ox + 8,  oy + 9,  5,  10, C_GUN_D);  // tip outer
    rect(ox + 10, oy + 11, 3,  6,  C_BLACK);  // barrel bore

    // Muzzle brake lines
    vline(ox + 20, oy + 9,  10, C_GUN_D);
    vline(ox + 30, oy + 9,  10, C_GUN_D);

    // Pump/forend (below barrel)
    rect(ox + 30, oy + 17, 32, 8, C_WOOD);    // wood pump
    rect(ox + 30, oy + 17, 32, 2, C_WOOD2);   // pump top
    rect(ox + 30, oy + 23, 32, 2, C_GUN_D);   // pump bottom
    // Pump grooves
    vline(ox + 38, oy + 18, 6, C_WOOD2);
    vline(ox + 46, oy + 18, 6, C_WOOD2);
    vline(ox + 54, oy + 18, 6, C_WOOD2);

    // Receiver / action body
    rect(ox + 70, oy + 8,  40, 22, C_GUN_M);   // main body
    rect(ox + 70, oy + 8,  40,  3, C_GUN_H);   // top edge
    rect(ox + 70, oy + 28, 40,  2, C_GUN_D);   // bottom edge
    // Ejection port
    rect(ox + 78, oy + 13, 20, 10, C_GUN_D);
    rect(ox + 80, oy + 14, 16,  8, C_BLACK);

    // Trigger guard
    rect(ox + 80, oy + 30, 20,  2, C_GUN_M);   // guard top
    rect(ox + 80, oy + 32, 2,  10, C_GUN_M);   // guard left
    rect(ox + 98, oy + 32, 2,  10, C_GUN_M);   // guard right
    rect(ox + 82, oy + 40, 16,  2, C_GUN_M);   // guard bottom
    // Trigger
    rect(ox + 88, oy + 31, 4,   8, C_GUN_D);

    // Stock (wood)
    rect(ox + 110, oy + 10, 30, 18, C_WOOD);    // stock body
    rect(ox + 110, oy + 10, 30,  3, C_WOOD2);   // top highlight
    rect(ox + 110, oy + 27, 30,  2, C_GUN_D);   // bottom shadow
    // Butt plate
    rect(ox + 138, oy + 9,  4,  20, C_GUN_M);
    rect(ox + 138, oy + 9,  4,   2, C_GUN_H);

    // Pistol grip
    rect(ox + 100, oy + 25, 16, 28, C_WOOD);    // grip body
    rect(ox + 100, oy + 25, 16,  3, C_WOOD2);   // grip top
    rect(ox + 114, oy + 25, 2,  28, C_GUN_D);   // grip right shadow
    // Grip texture dots
    for (int gy = oy+30; gy < oy+50; gy += 5) {
        for (int gx = ox+102; gx < ox+113; gx += 4) {
            pixel(gx, gy, C_WOOD2);
        }
    }

    // ─── MUZZLE FLASH ─────────────────────────────────────────
    if (g.player.flashTimer > 0.0f) {
        float t = g.player.flashTimer / FLASH_TIME;
        int fsize = (int)(14.0f * t);
        int fx = ox + 8;
        int fy = oy + 8;

        // Outer glow
        rect(fx - fsize,     fy - fsize/2, fsize*2, fsize, C_MUZZLE2);
        // Core cross
        rect(fx - fsize/2,   fy - fsize,   fsize,   fsize*2, C_MUZZLE);
        // Bright centre
        rect(fx - fsize/4,   fy - fsize/4, fsize/2, fsize/2, C_WHITE);
    }
}

// ═════════════════════════════════════════════════════════════
//  HUD  — status bar at bottom
// ═════════════════════════════════════════════════════════════

// Draw a pixel-art face based on health
static void drawFace(int health, int cx, int cy) {
    int r = 18;
    // Head outline
    rect(cx-r, cy-r, r*2, r*2, KDColor::RGB888(220,180,130));
    // Eyes
    KDColor eyeCol = (health > 50) ? KDColor::RGB888(0,100,255)
                                   : KDColor::RGB888(200,0,0);
    rect(cx-9, cy-6, 5, 5, C_WHITE);
    rect(cx+4,  cy-6, 5, 5, C_WHITE);
    rect(cx-8,  cy-5, 3, 3, eyeCol);
    rect(cx+5,  cy-5, 3, 3, eyeCol);
    // Nose
    rect(cx-1, cy,   2, 4, KDColor::RGB888(180,130,90));
    // Mouth (smile/frown by health)
    if (health > 75) {
        // Big smile
        hline(cx-7, cy+8, 14, C_BLOOD);
        hline(cx-9, cy+7, 3, C_BLOOD);
        hline(cx+6, cy+7, 3, C_BLOOD);
    } else if (health > 40) {
        // Neutral
        hline(cx-6, cy+8, 12, C_BLOOD);
    } else {
        // Frown
        hline(cx-7, cy+8, 14, C_BLOOD);
        hline(cx-9, cy+9, 3, C_BLOOD);
        hline(cx+6, cy+9, 3, C_BLOOD);
    }
    // Hair (bloody at low health)
    KDColor hairCol = (health < 25) ? C_BLOOD : C_BLACK;
    rect(cx-r, cy-r, r*2, 6, hairCol);
    // Head border
    for (int dx = -r; dx < r; dx++) pixel(cx+dx, cy-r, C_GUN_D);
    for (int dx = -r; dx < r; dx++) pixel(cx+dx, cy+r-1, C_GUN_D);
    for (int dy = -r; dy < r; dy++) pixel(cx-r, cy+dy, C_GUN_D);
    for (int dy = -r; dy < r; dy++) pixel(cx+r-1, cy+dy, C_GUN_D);
}

// Draw skull icon for ammo
static void drawSkull(int x, int y) {
    rect(x,   y,   14, 10, C_LGRAY);
    rect(x+2, y+10, 10,  2, C_LGRAY);
    rect(x+2, y+12, 4,   4, C_LGRAY);
    rect(x+8, y+12, 4,   4, C_LGRAY);
    rect(x+3, y+2,  3,   4, C_BLACK); // left eye
    rect(x+8, y+2,  3,   4, C_BLACK); // right eye
    rect(x+5, y+8,  4,   2, C_BLACK); // nose
}

void renderHUD(GameState& g) {
    const Player& p = g.player;

    // ─── HUD background ───────────────────────────────────────
    rect(0, HUD_Y, SCR_W, HUD_H, C_HUD_BG);

    // Divider line
    hline(0, HUD_Y, SCR_W, KDColor::RGB888(60, 30, 0));

    // Side panels
    rect(0,         HUD_Y+1, 80, HUD_H-1, C_HUD2);
    rect(80,        HUD_Y+1, 80, HUD_H-1, C_HUD3);
    rect(160,       HUD_Y+1, 80, HUD_H-1, C_HUD3);
    rect(240,       HUD_Y+1, 80, HUD_H-1, C_HUD2);

    // ─── HEALTH panel (left) ──────────────────────────────────
    // Label
    drawChar(10, 8, HUD_Y+3, 1, C_RED, C_HUD2);  // A
    drawChar(17, 14,HUD_Y+3, 1, C_RED, C_HUD2);  // L
    drawChar(18, 20,HUD_Y+3, 1, C_RED, C_HUD2);  // E

    // Bar background
    rect(5, HUD_Y+14, 70, 10, C_DGRAY);
    // Bar fill
    int hpBar = (p.health * 70) / MAX_HEALTH;
    KDColor hpCol = (p.health > 60) ? C_GREEN :
                    (p.health > 30) ? C_YELLOW : C_RED;
    rect(5, HUD_Y+14, hpBar, 10, hpCol);
    // Bar border
    for (int bx = 5; bx < 75; bx++) {
        pixel(bx, HUD_Y+14, C_GUN_M);
        pixel(bx, HUD_Y+23, C_GUN_M);
    }
    vline(5,  HUD_Y+14, 10, C_GUN_M);
    vline(74, HUD_Y+14, 10, C_GUN_M);

    // Number
    drawNumber(p.health, 12, HUD_Y+28, 2, hpCol, C_HUD2, 3);

    // ─── FACE (centre-left) ───────────────────────────────────
    drawFace(p.health, 120, HUD_Y + 30);

    // ─── SCORE ────────────────────────────────────────────────
    // "SCORE" label using individual chars
    drawChar(13, 165, HUD_Y+4, 1, C_YELLOW, C_HUD3); // S
    drawChar(14, 171, HUD_Y+4, 1, C_YELLOW, C_HUD3); // C
    drawChar(12, 177, HUD_Y+4, 1, C_YELLOW, C_HUD3); // O
    drawChar(18, 183, HUD_Y+4, 1, C_YELLOW, C_HUD3); // E (R approximated)
    drawNumber(p.score, 162, HUD_Y+16, 2, C_YELLOW, C_HUD3, 5);

    // Kills
    drawChar(15, 162, HUD_Y+34, 1, C_LGRAY, C_HUD3); // K
    drawChar(16, 168, HUD_Y+34, 1, C_LGRAY, C_HUD3); // I
    drawChar(17, 174, HUD_Y+34, 1, C_LGRAY, C_HUD3); // L
    drawChar(17, 180, HUD_Y+34, 1, C_LGRAY, C_HUD3); // L
    drawChar(13, 186, HUD_Y+34, 1, C_LGRAY, C_HUD3); // S
    drawNumber(g.kills, 162, HUD_Y+44, 2, C_LGRAY, C_HUD3, 2);

    // ─── AMMO panel (right) ───────────────────────────────────
    // Skull icon
    drawSkull(245, HUD_Y+6);

    // Ammo bar
    rect(242, HUD_Y+22, 70, 10, C_DGRAY);
    int ammoBar = (p.ammo * 70) / MAX_AMMO;
    KDColor ammoCol = (p.ammo > 20) ? C_ORANGE :
                      (p.ammo > 8)  ? C_YELLOW : C_RED;
    rect(242, HUD_Y+22, ammoBar, 10, ammoCol);
    for (int bx = 242; bx < 312; bx++) {
        pixel(bx, HUD_Y+22, C_GUN_M);
        pixel(bx, HUD_Y+31, C_GUN_M);
    }
    vline(242, HUD_Y+22, 10, C_GUN_M);
    vline(311, HUD_Y+22, 10, C_GUN_M);

    drawNumber(p.ammo, 248, HUD_Y+36, 2, ammoCol, C_HUD2, 2);

    // ─── Crosshair ────────────────────────────────────────────
    int cx = SCR_W / 2;
    int cy = VIEW_H / 2;
    // Dot crosshair
    pixel(cx,   cy,   C_RED);
    pixel(cx-2, cy,   C_RED);
    pixel(cx+2, cy,   C_RED);
    pixel(cx,   cy-2, C_RED);
    pixel(cx,   cy+2, C_RED);

    // ─── Hurt flash overlay ───────────────────────────────────
    if (g.player.hurtTimer > 0.0f) {
        float alpha = g.player.hurtTimer / HURT_TIME;
        // Draw semi-transparent red border
        int thick = (int)(alpha * 10.0f) + 1;
        rect(0,              0,       SCR_W, thick,  C_DRED);
        rect(0,              VIEW_H-thick, SCR_W, thick,  C_DRED);
        rect(0,              0,       thick, VIEW_H, C_DRED);
        rect(SCR_W-thick,    0,       thick, VIEW_H, C_DRED);
    }
}

// ═════════════════════════════════════════════════════════════
//  TITLE / GAME-OVER SCREENS
// ═════════════════════════════════════════════════════════════

void renderTitleScreen() {
    KDContext* c = KDIonContext::sharedContext();
    c->fillRect(KDRect(0,0,SCR_W,SCR_H), C_BLACK);

    // Big DOOM text (manual pixel art 40px tall)
    // D
    rect(30,  40, 8, 60, C_RED);
    rect(38,  40, 20, 8, C_RED);
    rect(38,  92, 20, 8, C_RED);
    rect(58,  48, 8, 44, C_RED);
    // O
    rect(80,  40, 32, 8, C_RED);
    rect(80,  92, 32, 8, C_RED);
    rect(80,  40, 8,  60, C_RED);
    rect(104, 40, 8,  60, C_RED);
    // O (second)
    rect(126, 40, 32, 8, C_RED);
    rect(126, 92, 32, 8, C_RED);
    rect(126, 40, 8,  60, C_RED);
    rect(150, 40, 8,  60, C_RED);
    // M
    rect(172, 40, 8,  60, C_RED);
    rect(180, 48, 8,  8,  C_RED);
    rect(188, 56, 8,  8,  C_RED);
    rect(196, 48, 8,  8,  C_RED);
    rect(204, 40, 8,  60, C_RED);

    // Tagline
    c->drawString("  NUMWORKS EDITION", KDPoint(60, 115),
                  KDFont::SmallFont, C_YELLOW, C_BLACK);

    // Hell image (procedural pixel art)
    // Ground
    rect(0, 145, SCR_W, 50, KDColor::RGB888(20,5,0));
    // Lava glow lines
    for (int i = 0; i < 8; i++) {
        rect(0, 155+i*3, SCR_W, 2,
             KDColor::RGB888(180-i*20, 20, 0));
    }
    // Skull row
    for (int i = 0; i < 8; i++) {
        drawSkull(20 + i*38, 148);
    }
    // Fire particles
    for (int i = 0; i < 40; i++) {
        int fx = (i * 97 + 13) % SCR_W;
        int fy = 130 + (i * 53) % 20;
        int fh = 5 + (i * 37) % 15;
        KDColor fc = (i % 3 == 0) ? C_MUZZLE :
                     (i % 3 == 1) ? C_ORANGE : C_RED;
        rect(fx, fy - fh, 3, fh, fc);
    }

    // Controls
    c->drawString("UP/DOWN : Move",    KDPoint(20, 200), KDFont::SmallFont, C_GRAY, C_BLACK);
    c->drawString("LEFT/RIGHT : Turn", KDPoint(20, 212), KDFont::SmallFont, C_GRAY, C_BLACK);
    c->drawString("OK : Fire",         KDPoint(20, 224), KDFont::SmallFont, C_GRAY, C_BLACK);
    c->drawString("Press OK to start", KDPoint(160, 212),KDFont::SmallFont, C_YELLOW, C_BLACK);
}

void renderGameOver(const GameState& g) {
    KDContext* c = KDIonContext::sharedContext();
    c->fillRect(KDRect(0,0,SCR_W,SCR_H), KDColor::RGB888(10,0,0));

    if (g.won) {
        c->drawString("   YOU WIN!", KDPoint(80, 80), KDFont::LargeFont, C_YELLOW, KDColor::RGB888(10,0,0));
        c->drawString("All demons defeated!", KDPoint(50, 120), KDFont::SmallFont, C_GREEN, KDColor::RGB888(10,0,0));
    } else {
        c->drawString("  YOU DIED", KDPoint(80, 80), KDFont::LargeFont, C_RED, KDColor::RGB888(10,0,0));
        c->drawString("The demons got you...", KDPoint(45, 120), KDFont::SmallFont, C_GRAY, KDColor::RGB888(10,0,0));
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "Score: %d", g.player.score);
    c->drawString(buf, KDPoint(100, 150), KDFont::SmallFont, C_YELLOW, KDColor::RGB888(10,0,0));
    snprintf(buf, sizeof(buf), "Kills: %d/%d", g.kills, g.numEnemies);
    c->drawString(buf, KDPoint(100, 165), KDFont::SmallFont, C_LGRAY, KDColor::RGB888(10,0,0));

    c->drawString("Press OK to restart", KDPoint(60, 210), KDFont::SmallFont, C_WHITE, KDColor::RGB888(10,0,0));
    c->drawString("Press BACK to quit",  KDPoint(60, 224), KDFont::SmallFont, C_GRAY, KDColor::RGB888(10,0,0));
}
