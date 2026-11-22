#pragma once

#include "Types.hpp"
#include <vector>

namespace pvz {

class Game {
public:
    Game();
    void Init();
    void Update(float dt);
    void Draw();
    bool ShouldQuit() const { return shouldQuit; }

private:
    // 状态管理
    GameState state = GameState::MENU;
    bool shouldQuit = false;
    bool isPaused = false;
    bool showAlmanac = false;
    bool showDevGui = false;
    int difficultyIdx = 0; // 0: Normal, 1: Hard, 2: Endless

    // 游戏数值
    int sun = 150;
    int score = 0;
    int selectedCard = -1;
    bool shovelSelected = false;
    float gameSpeed = 1.0f;
    bool speed2x = false;

    // 调试 / GUI 参数
    bool showDebugInfo = false;
    bool instantCooldown = false;
    float spawnRateMultiplier = 1.0f;

    // 波次控制
    float skySunTimer = 6.0f;
    float spawnTimer = 4.0f;
    int zombiesSpawned = 0;
    int totalZombiesInLevel = 22;
    int currentWave = 1;
    float bannerTimer = 0.0f;
    std::string bannerText;

    // 实体容器
    std::vector<SeedCard> cards;
    std::vector<Plant> plants;
    std::vector<Zombie> zombies;
    std::vector<Projectile> projectiles;
    std::vector<SunOrb> sunOrbs;
    std::vector<LawnMower> mowers;
    std::vector<Particle> particles;

    // 逻辑函数
    void StartNewGame();
    void InitCards();
    void InitMowers();
    void UpdatePlaying(float dt);
    void HandleInput();
    void SpawnZombie(ZombieType type, int row);
    void SpawnSkySun();
    void SpawnParticles(float x, float y, Color color, int count, float speed);
    bool IsCellOccupied(int row, int col) const;
    Plant* GetPlantAt(int row, int col);
    void PlantAt(int row, int col, PlantType type);
    void RemovePlantAt(int row, int col);

    // 渲染与 GUI 函数
    void DrawLawn();
    void DrawMowers();
    void DrawPlants();
    void DrawZombies();
    void DrawProjectiles();
    void DrawSunOrbs();
    void DrawParticles();
    void DrawTopHUD();
    void DrawPlacementPreview();
    void DrawMenuGUI();
    void DrawAlmanacWindow();
    void DrawDevGuiWindow();
    void DrawPauseOrEndGUI();
};

} // namespace pvz