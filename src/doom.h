#pragma once

// ─────────────────────────────────────────────
//  DOOM-NW  ·  A Doom-inspired raycaster
//  for NumWorks graphing calculators
//  Target : Epsilon >= 16  (N0110 / N0120)
//  Screen : 320 × 240 px
// ─────────────────────────────────────────────

#include <kandinsky.h>
#include <ion.h>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>

// ===== SCREEN ================================================
static constexpr int SCR_W  = 320;
static constexpr int SCR_H  = 240;
static constexpr int VIEW_H = 180;   // game viewport height
static constexpr int HUD_Y  = 180;   // HUD top edge
static constexpr int HUD_H  =  60;   // HUD height

// ===== GAME TUNING ===========================================
static constexpr float MOVE_SPEED    = 0.07f;
static constexpr float ROT_SPEED     = 0.048f;
static constexpr float SHOOT_DELAY   = 0.35f; // seconds between shots
static constexpr float FLASH_TIME    = 0.12f;
static constexpr float HURT_TIME     = 0.20f;
static constexpr float ENEMY_SPEED   = 0.025f;
static constexpr float ENEMY_SHOOT_CD= 1.8f;
static constexpr int   PLAYER_DMG    = 30;    // damage per shot to enemy
static constexpr int   ENEMY_DMG     = 8;     // damage per shot to player
static constexpr int   MAX_HEALTH    = 100;
static constexpr int   MAX_AMMO      = 99;
static constexpr int   MAX_ENEMIES   = 8;

// ===== COLORS ================================================
static const KDColor C_BLACK   = KDColor::RGB24(0x000000);
static const KDColor C_WHITE   = KDColor::RGB24(0xFFFFFF);
static const KDColor C_CEIL    = KDColor::RGB24(0x080812);
static const KDColor C_FLOOR   = KDColor::RGB24(0x18100A);
static const KDColor C_W1_NS   = KDColor::RGB24(0x7A1A1A); // wall type1 N/S face
static const KDColor C_W1_EW   = KDColor::RGB24(0xB02828); // wall type1 E/W face
static const KDColor C_W2_NS   = KDColor::RGB24(0x4A3010); // wall type2 N/S face
static const KDColor C_W2_EW   = KDColor::RGB24(0x6A4818); // wall type2 E/W face
static const KDColor C_HUD_BG  = KDColor::RGB24(0x0C0800);
static const KDColor C_HUD2    = KDColor::RGB24(0x1A1000);
static const KDColor C_HUD3    = KDColor::RGB24(0x241800);
static const KDColor C_RED     = KDColor::RGB24(0xFF2020);
static const KDColor C_DRED    = KDColor::RGB24(0x880000);
static const KDColor C_GREEN   = KDColor::RGB24(0x00CC00);
static const KDColor C_DGREEN  = KDColor::RGB24(0x006600);
static const KDColor C_YELLOW  = KDColor::RGB24(0xFFCC00);
static const KDColor C_ORANGE  = KDColor::RGB24(0xFF6600);
static const KDColor C_GRAY    = KDColor::RGB24(0x888888);
static const KDColor C_DGRAY   = KDColor::RGB24(0x333333);
static const KDColor C_LGRAY   = KDColor::RGB24(0xBBBBBB);
static const KDColor C_MUZZLE  = KDColor::RGB24(0xFFAA00);
static const KDColor C_MUZZLE2 = KDColor::RGB24(0xFF5500);
static const KDColor C_GUN_D   = KDColor::RGB24(0x1C1C1C);
static const KDColor C_GUN_M   = KDColor::RGB24(0x4A4A4A);
static const KDColor C_GUN_L   = KDColor::RGB24(0x888888);
static const KDColor C_GUN_H   = KDColor::RGB24(0xAAAAAA);
static const KDColor C_WOOD    = KDColor::RGB24(0x5C3010);
static const KDColor C_WOOD2   = KDColor::RGB24(0x7A4018);
static const KDColor C_ENM_BDY = KDColor::RGB24(0x6B2D0A); // enemy body
static const KDColor C_ENM_SK  = KDColor::RGB24(0xD4946A); // enemy skin
static const KDColor C_ENM_EYE = KDColor::RGB24(0xFF0000); // enemy eyes
static const KDColor C_ENM_CLO = KDColor::RGB24(0x2A2A6A); // enemy clothes
static const KDColor C_BLOOD   = KDColor::RGB24(0xAA0000);

// ===== MAP ===================================================
#define MAP_W 20
#define MAP_H 20

// 0=empty  1=red brick  2=brown stone
static const uint8_t WORLD_MAP[MAP_H][MAP_W] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,2,2,2,0,0,0,0,0,0,2,2,2,0,0,0,0,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,1},
    {1,0,0,2,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,1},
    {1,0,0,2,2,2,0,0,0,0,0,0,2,2,2,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

// ===== STRUCTURES ============================================

struct Player {
    float x, y;          // world position
    float dirX, dirY;    // direction vector (unit)
    float planeX, planeY;// camera plane (perpendicular to dir, len=tan(FOV/2))
    int   health;
    int   ammo;
    int   score;
    bool  alive;
    float shootCD;       // shoot cooldown
    float flashTimer;    // muzzle flash timer
    float hurtTimer;     // red screen flash when hit
    bool  shooting;      // animating shot this frame
};

struct Enemy {
    float x, y;
    int   health;
    bool  alive;
    int   state;         // 0=idle  1=chase  2=attack  3=dying
    float stateTimer;
    float shootTimer;
    // --- per-frame render data (set during sprite sort) ---
    float dist;
    int   screenX;
    int   spriteH;       // projected screen height
};

struct GameState {
    Player  player;
    Enemy   enemies[MAX_ENEMIES];
    int     numEnemies;
    float   zBuffer[SCR_W]; // perpendicular wall distance per column
    bool    gameOver;
    bool    won;
    int     frame;
    int     kills;
};

// ===== FUNCTION DECLARATIONS =================================

// game.cpp
void  initGame(GameState& g);
void  processInput(GameState& g);
void  updateGame(GameState& g);
bool  hasLineOfSight(const GameState& g,
                     float x1,float y1,float x2,float y2);

// raycaster.cpp
void  renderWalls(GameState& g);

// renderer.cpp
void  renderEnemies(GameState& g);
void  renderWeapon(GameState& g);
void  renderHUD(GameState& g);
void  renderGameOver(const GameState& g);
void  renderTitleScreen();

// helpers (inline)
inline KDCoordinate clamp(int v, int lo, int hi) {
    if (v < lo) return (KDCoordinate)lo;
    if (v > hi) return (KDCoordinate)hi;
    return (KDCoordinate)v;
}
inline float fclamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
