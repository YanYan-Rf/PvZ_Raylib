#pragma once

#include <raylib.h>
#include <string>
#include <vector>

namespace pvz {

constexpr int SCREEN_W = 1280;
constexpr int SCREEN_H = 720;

constexpr int GRID_ROWS = 5;
constexpr int GRID_COLS = 9;
constexpr float GRID_X = 210.0f;
constexpr float GRID_Y = 140.0f;
constexpr float CELL_W = 105.0f;
constexpr float CELL_H = 105.0f;

enum class GameState {
    MENU,
    PLAYING,
    VICTORY,
    GAME_OVER
};

enum class Difficulty {
    NORMAL = 0,
    HARD = 1,
    ENDLESS = 2
};

enum class PlantType {
    NONE = -1,
    SUNFLOWER = 0,
    PEASHOOTER = 1,
    WALLNUT = 2,
    SNOWPEA = 3,
    CHERRYBOMB = 4
};

enum class ZombieType {
    NORMAL = 0,
    CONEHEAD = 1,
    BUCKETHEAD = 2
};

enum class ProjectileType {
    PEA = 0,
    ICE_PEA = 1
};

struct SeedCard {
    PlantType type;
    std::string name;
    int cost;
    float maxCooldown;
    float cooldownTimer;
    Color cardColor;
};

struct Plant {
    PlantType type;
    int row;
    int col;
    float hp;
    float maxHp;
    float actionTimer;
    float hitFlashTimer = 0.0f;
};

struct Zombie {
    ZombieType type;
    int row;
    float x;
    float hp;
    float maxHp;
    float speed;
    float dps;
    float slowTimer = 0.0f;
    float hitFlashTimer = 0.0f;
    float animPhase = 0.0f;
    bool isEating = false;
};

struct Projectile {
    ProjectileType type;
    int row;
    float x;
    float y;
    float speed;
    float damage;
    bool alive = true;
};

struct SunOrb {
    float x;
    float y;
    float targetY;
    int value = 25;
    float lifeTimer = 10.0f;
    bool collected = false;
};

struct LawnMower {
    int row;
    float x;
    bool active = true;
    bool triggered = false;
};

struct Particle {
    float x;
    float y;
    float vx;
    float vy;
    float radius;
    float life;
    float maxLife;
    Color color;
};

} // namespace pvz