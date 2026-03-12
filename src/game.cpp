#include "doom.h"

// ─────────────────────────────────────────────────────────────
//  GAME  —  init / input / update / AI
// ─────────────────────────────────────────────────────────────

// Starting spawn positions for enemies (world units)
static const float ENEMY_SPAWN_X[MAX_ENEMIES] = {
    3.5f, 16.5f, 3.5f, 16.5f,
    9.5f, 10.5f,  5.5f, 14.5f
};
static const float ENEMY_SPAWN_Y[MAX_ENEMIES] = {
    4.5f,  4.5f, 14.5f, 14.5f,
    3.5f, 16.5f,  9.5f,  9.5f
};

// ── Init ─────────────────────────────────────────────────────
void initGame(GameState& g) {
    // Player starts in top-left open area, facing south-east
    g.player.x      = 2.5f;
    g.player.y      = 2.5f;
    g.player.dirX   =  0.0f;
    g.player.dirY   =  1.0f;
    // camera plane length = tan(FOV/2) ≈ 0.66 for ~66° FOV
    g.player.planeX =  0.66f;
    g.player.planeY =  0.0f;
    g.player.health = MAX_HEALTH;
    g.player.ammo   = 30;
    g.player.score  = 0;
    g.player.alive  = true;
    g.player.shootCD    = 0.0f;
    g.player.flashTimer = 0.0f;
    g.player.hurtTimer  = 0.0f;
    g.player.shooting   = false;

    g.numEnemies = MAX_ENEMIES;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        g.enemies[i].x          = ENEMY_SPAWN_X[i];
        g.enemies[i].y          = ENEMY_SPAWN_Y[i];
        g.enemies[i].health     = 60;
        g.enemies[i].alive      = true;
        g.enemies[i].state      = 0;
        g.enemies[i].stateTimer = 0.0f;
        g.enemies[i].shootTimer = ENEMY_SHOOT_CD * (0.5f + (float)i * 0.15f);
    }

    g.gameOver = false;
    g.won      = false;
    g.frame    = 0;
    g.kills    = 0;

    memset(g.zBuffer, 0, sizeof(g.zBuffer));
}

// ── Line-of-sight check (DDA in world space) ─────────────────
bool hasLineOfSight(const GameState& g,
                    float x1, float y1,
                    float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 0.001f) return true;
    dx /= len; dy /= len;

    const int STEPS = (int)(len * 10.0f);
    float step = len / (float)STEPS;
    float cx = x1, cy = y1;
    for (int i = 0; i < STEPS; i++) {
        cx += dx * step;
        cy += dy * step;
        int mx = (int)cx;
        int my = (int)cy;
        if (mx < 0 || mx >= MAP_W || my < 0 || my >= MAP_H) return false;
        if (WORLD_MAP[my][mx] != 0) return false;
    }
    return true;
}

// ── Input & movement ─────────────────────────────────────────
void processInput(GameState& g) {
    if (!g.player.alive || g.gameOver) return;

    Ion::Keyboard::State keys = Ion::Keyboard::scan();
    Player& p = g.player;

    // Move forward / backward
    if (keys.keyDown(Ion::Keyboard::Key::Up)) {
        float nx = p.x + p.dirX * MOVE_SPEED;
        float ny = p.y + p.dirY * MOVE_SPEED;
        // wall slide — try both axes independently
        if (WORLD_MAP[(int)p.y][(int)nx] == 0) p.x = nx;
        if (WORLD_MAP[(int)ny][(int)p.x] == 0) p.y = ny;
    }
    if (keys.keyDown(Ion::Keyboard::Key::Down)) {
        float nx = p.x - p.dirX * MOVE_SPEED;
        float ny = p.y - p.dirY * MOVE_SPEED;
        if (WORLD_MAP[(int)p.y][(int)nx] == 0) p.x = nx;
        if (WORLD_MAP[(int)ny][(int)p.x] == 0) p.y = ny;
    }

    // Rotate left / right
    if (keys.keyDown(Ion::Keyboard::Key::Left)) {
        float cos_a =  cosf(-ROT_SPEED);
        float sin_a =  sinf(-ROT_SPEED);
        float od = p.dirX;
        p.dirX   = p.dirX   * cos_a - p.dirY   * sin_a;
        p.dirY   = od        * sin_a + p.dirY   * cos_a;
        float op = p.planeX;
        p.planeX = p.planeX  * cos_a - p.planeY * sin_a;
        p.planeY = op         * sin_a + p.planeY * cos_a;
    }
    if (keys.keyDown(Ion::Keyboard::Key::Right)) {
        float cos_a = cosf(ROT_SPEED);
        float sin_a = sinf(ROT_SPEED);
        float od = p.dirX;
        p.dirX   = p.dirX   * cos_a - p.dirY   * sin_a;
        p.dirY   = od        * sin_a + p.dirY   * cos_a;
        float op = p.planeX;
        p.planeX = p.planeX  * cos_a - p.planeY * sin_a;
        p.planeY = op         * sin_a + p.planeY * cos_a;
    }

    // Shoot  (OK key = EXE on NumWorks)
    if (keys.keyDown(Ion::Keyboard::Key::OK) && p.shootCD <= 0.0f) {
        if (p.ammo > 0) {
            p.ammo--;
            p.shootCD    = SHOOT_DELAY;
            p.flashTimer = FLASH_TIME;
            p.shooting   = true;

            // Hitscan — find closest enemy in centre column
            float bestDist = 1e9f;
            int   bestIdx  = -1;
            for (int i = 0; i < g.numEnemies; i++) {
                if (!g.enemies[i].alive) continue;
                Enemy& e = g.enemies[i];
                // Check if enemy is roughly in front
                float ex = e.x - p.x;
                float ey = e.y - p.y;
                float dot = ex * p.dirX + ey * p.dirY;
                if (dot < 0.3f) continue;
                float dist = sqrtf(ex*ex + ey*ey);
                // Is it on-screen near centre?
                if (e.screenX > SCR_W/2 - 30 && e.screenX < SCR_W/2 + 30) {
                    // Not behind a wall
                    if (dist < g.zBuffer[e.screenX > 0 && e.screenX < SCR_W
                                         ? e.screenX : SCR_W/2]) {
                        if (dist < bestDist) {
                            bestDist = dist;
                            bestIdx  = i;
                        }
                    }
                }
            }
            if (bestIdx >= 0) {
                g.enemies[bestIdx].health -= PLAYER_DMG;
                if (g.enemies[bestIdx].health <= 0) {
                    g.enemies[bestIdx].alive = false;
                    g.enemies[bestIdx].state = 3; // dying
                    g.kills++;
                    p.score += 100;
                }
            }
        }
    }

    // Quit
    if (keys.keyDown(Ion::Keyboard::Key::Back)) {
        g.gameOver = true;
    }
}

// ── Update (AI, timers, win/lose) ────────────────────────────
void updateGame(GameState& g) {
    const float dt = 1.0f / 30.0f; // assume ~30 fps
    Player& p = g.player;

    // Update timers
    if (p.shootCD    > 0.0f) p.shootCD    -= dt;
    if (p.flashTimer > 0.0f) p.flashTimer -= dt;
    if (p.hurtTimer  > 0.0f) p.hurtTimer  -= dt;
    p.shooting = false;

    // Check win condition
    int alive = 0;
    for (int i = 0; i < g.numEnemies; i++)
        if (g.enemies[i].alive) alive++;
    if (alive == 0 && !g.gameOver) {
        g.won = true;
        g.gameOver = true;
        return;
    }

    // Enemy AI
    for (int i = 0; i < g.numEnemies; i++) {
        Enemy& e = g.enemies[i];
        if (!e.alive) continue;

        float dx = p.x - e.x;
        float dy = p.y - e.y;
        float dist = sqrtf(dx*dx + dy*dy);

        e.stateTimer -= dt;

        // State machine
        switch (e.state) {
            case 0: // idle
                if (dist < 8.0f && hasLineOfSight(g, e.x, e.y, p.x, p.y)) {
                    e.state = 1; // start chasing
                }
                break;

            case 1: // chase
                if (dist > 12.0f) { e.state = 0; break; }
                if (dist < 3.5f)  { e.state = 2; break; } // close enough to shoot

                // Move towards player
                if (dist > 0.01f) {
                    float nx = e.x + (dx/dist) * ENEMY_SPEED;
                    float ny = e.y + (dy/dist) * ENEMY_SPEED;
                    if (WORLD_MAP[(int)e.y][(int)nx] == 0) e.x = nx;
                    if (WORLD_MAP[(int)ny][(int)e.x] == 0) e.y = ny;
                }
                break;

            case 2: // attack / shoot
                if (dist > 5.5f) { e.state = 1; break; }

                e.shootTimer -= dt;
                if (e.shootTimer <= 0.0f && hasLineOfSight(g, e.x, e.y, p.x, p.y)) {
                    e.shootTimer = ENEMY_SHOOT_CD;
                    // Damage player
                    p.health -= ENEMY_DMG;
                    p.hurtTimer = HURT_TIME;
                    if (p.health <= 0) {
                        p.health = 0;
                        p.alive  = false;
                        g.gameOver = true;
                    }
                }
                break;

            case 3: // dying — corpse stays for 2s then disappears
                e.stateTimer -= dt;
                if (e.stateTimer < -2.0f) e.alive = false;
                break;
        }
    }

    g.frame++;
}
