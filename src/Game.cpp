#include "Game.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

// 仅在此编译单元实现 raygui，避免多文件链接重定义
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>

namespace pvz {

namespace {

float RandRange(float minVal, float maxVal) {
    return minVal + static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f * (maxVal - minVal);
}

Rectangle GetDevWindowRect() {
    return {SCREEN_W - 285.0f, 145.0f, 270.0f, 285.0f};
}

} // namespace

Game::Game() {
    Init();
}

void Game::Init() {
    GuiLoadStyleDefault();
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
    InitCards();
    InitMowers();
}

void Game::InitCards() {
    cards = {
        {PlantType::SUNFLOWER,  "Sunflower",   50,  5.0f,  0.0f, {240, 200, 70, 255}},
        {PlantType::PEASHOOTER, "Peashooter",  100, 5.0f,  0.0f, {110, 200, 95, 255}},
        {PlantType::WALLNUT,    "Wall-nut",    50,  12.0f, 0.0f, {195, 145, 90, 255}},
        {PlantType::SNOWPEA,    "Snow Pea",    175, 7.0f,  0.0f, {110, 210, 240, 255}},
        {PlantType::CHERRYBOMB, "Cherry Bomb", 150, 18.0f, 0.0f, {225, 85, 85, 255}}
    };
}

void Game::InitMowers() {
    mowers.clear();
    for (int r = 0; r < GRID_ROWS; ++r) {
        mowers.push_back({r, GRID_X - 55.0f, true, false});
    }
}

void Game::StartNewGame() {
    state = GameState::PLAYING;
    isPaused = false;
    selectedCard = -1;
    shovelSelected = false;
    sun = (difficultyIdx == 1) ? 125 : 150;
    score = 0;

    skySunTimer = 5.0f;
    spawnTimer = 4.5f;
    zombiesSpawned = 0;
    currentWave = 1;
    totalZombiesInLevel = (difficultyIdx == 0) ? 22 : (difficultyIdx == 1 ? 34 : 999999);
    bannerText = "ZOMBIES ARE COMING!";
    bannerTimer = 2.5f;

    plants.clear();
    zombies.clear();
    projectiles.clear();
    sunOrbs.clear();
    particles.clear();
    InitCards();
    InitMowers();
}

bool Game::IsCellOccupied(int row, int col) const {
    for (const auto& p : plants) {
        if (p.row == row && p.col == col && p.hp > 0) return true;
    }
    return false;
}

Plant* Game::GetPlantAt(int row, int col) {
    for (auto& p : plants) {
        if (p.row == row && p.col == col && p.hp > 0) return &p;
    }
    return nullptr;
}

void Game::PlantAt(int row, int col, PlantType type) {
    Plant p{};
    p.type = type;
    p.row = row;
    p.col = col;
    switch (type) {
        case PlantType::SUNFLOWER:
            p.maxHp = 300.0f;
            p.actionTimer = 5.5f;
            break;
        case PlantType::PEASHOOTER:
            p.maxHp = 300.0f;
            p.actionTimer = 0.8f;
            break;
        case PlantType::WALLNUT:
            p.maxHp = 1400.0f;
            p.actionTimer = 0.0f;
            break;
        case PlantType::SNOWPEA:
            p.maxHp = 300.0f;
            p.actionTimer = 0.8f;
            break;
        case PlantType::CHERRYBOMB:
            p.maxHp = 9999.0f;
            p.actionTimer = 0.7f; // 爆炸引信时间
            break;
        default:
            break;
    }
    p.hp = p.maxHp;
    plants.push_back(p);

    float cx = GRID_X + col * CELL_W + CELL_W * 0.5f;
    float cy = GRID_Y + row * CELL_H + CELL_H * 0.5f;
    SpawnParticles(cx, cy, {130, 210, 90, 255}, 10, 90.0f);
}

void Game::RemovePlantAt(int row, int col) {
    for (auto& p : plants) {
        if (p.row == row && p.col == col && p.hp > 0) {
            p.hp = 0.0f;
            float cx = GRID_X + col * CELL_W + CELL_W * 0.5f;
            float cy = GRID_Y + row * CELL_H + CELL_H * 0.5f;
            SpawnParticles(cx, cy, {140, 100, 60, 255}, 10, 80.0f);
            break;
        }
    }
}

void Game::SpawnZombie(ZombieType type, int row) {
    Zombie z{};
    z.type = type;
    z.row = row;
    z.x = SCREEN_W + 30.0f;
    z.animPhase = RandRange(0.0f, 6.28f);

    float hardScale = (difficultyIdx == 1) ? 1.25f : (difficultyIdx == 2 ? 1.0f + currentWave * 0.08f : 1.0f);
    switch (type) {
        case ZombieType::NORMAL:
            z.maxHp = 200.0f * hardScale;
            z.speed = 24.0f;
            z.dps = 55.0f;
            break;
        case ZombieType::CONEHEAD:
            z.maxHp = 480.0f * hardScale;
            z.speed = 23.0f;
            z.dps = 60.0f;
            break;
        case ZombieType::BUCKETHEAD:
            z.maxHp = 900.0f * hardScale;
            z.speed = 21.0f;
            z.dps = 65.0f;
            break;
    }
    z.hp = z.maxHp;
    zombies.push_back(z);
}

void Game::SpawnSkySun() {
    SunOrb orb{};
    orb.x = RandRange(GRID_X + 40.0f, GRID_X + GRID_COLS * CELL_W - 40.0f);
    orb.y = 110.0f;
    orb.targetY = RandRange(GRID_Y + 60.0f, GRID_Y + GRID_ROWS * CELL_H - 50.0f);
    orb.value = 25;
    orb.lifeTimer = 11.0f;
    sunOrbs.push_back(orb);
}

void Game::SpawnParticles(float x, float y, Color color, int count, float speed) {
    for (int i = 0; i < count; ++i) {
        float angle = RandRange(0.0f, 6.28318f);
        float spd = RandRange(speed * 0.35f, speed);
        Particle pt{};
        pt.x = x;
        pt.y = y;
        pt.vx = std::cos(angle) * spd;
        pt.vy = std::sin(angle) * spd;
        pt.radius = RandRange(3.0f, 6.5f);
        pt.maxLife = RandRange(0.25f, 0.55f);
        pt.life = pt.maxLife;
        pt.color = color;
        particles.push_back(pt);
    }
}

void Game::Update(float dt) {
    if (state == GameState::PLAYING) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
            isPaused = !isPaused;
        }
        gameSpeed = speed2x ? 2.0f : 1.0f;

        if (!isPaused) {
            HandleInput();
            UpdatePlaying(dt * gameSpeed);
        }
    }
}

void Game::HandleInput() {
    // 快捷键选择植物与铲子
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        if (IsKeyPressed(KEY_ONE + i)) {
            if (sun >= cards[i].cost && cards[i].cooldownTimer <= 0.0f) {
                selectedCard = (selectedCard == i) ? -1 : i;
                shovelSelected = false;
            }
        }
    }
    if (IsKeyPressed(KEY_S)) {
        shovelSelected = !shovelSelected;
        selectedCard = -1;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        selectedCard = -1;
        shovelSelected = false;
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;

    Vector2 mouse = GetMousePosition();

    // 若打开了 Dev GUI 窗口且点击落在窗口内，忽略棋盘交互
    if (showDevGui && CheckCollisionPointRec(mouse, GetDevWindowRect())) {
        return;
    }

    // 1. 优先拾取阳光
    for (auto& orb : sunOrbs) {
        if (!orb.collected && CheckCollisionPointCircle(mouse, {orb.x, orb.y}, 32.0f)) {
            orb.collected = true;
            sun += orb.value;
            score += 10;
            SpawnParticles(orb.x, orb.y, YELLOW, 8, 75.0f);
            return;
        }
    }

    // 2. 点击顶部植物卡片
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        Rectangle cardRec = {140.0f + i * 96.0f, 14.0f, 88.0f, 96.0f};
        if (CheckCollisionPointRec(mouse, cardRec)) {
            if (sun >= cards[i].cost && cards[i].cooldownTimer <= 0.0f) {
                selectedCard = (selectedCard == i) ? -1 : i;
                shovelSelected = false;
            }
            return;
        }
    }

    // 3. 点击铲子按钮
    Rectangle shovelRec = {140.0f + cards.size() * 96.0f + 10.0f, 22.0f, 76.0f, 80.0f};
    if (CheckCollisionPointRec(mouse, shovelRec)) {
        shovelSelected = !shovelSelected;
        selectedCard = -1;
        return;
    }

    // 4. 在草坪网格种植或铲除
    if (mouse.x >= GRID_X && mouse.x < GRID_X + GRID_COLS * CELL_W &&
        mouse.y >= GRID_Y && mouse.y < GRID_Y + GRID_ROWS * CELL_H) {
        int col = static_cast<int>((mouse.x - GRID_X) / CELL_W);
        int row = static_cast<int>((mouse.y - GRID_Y) / CELL_H);

        if (shovelSelected) {
            if (IsCellOccupied(row, col)) {
                RemovePlantAt(row, col);
                shovelSelected = false;
            }
        } else if (selectedCard >= 0) {
            auto& card = cards[selectedCard];
            if (!IsCellOccupied(row, col) && sun >= card.cost && card.cooldownTimer <= 0.0f) {
                PlantAt(row, col, card.type);
                sun -= card.cost;
                card.cooldownTimer = instantCooldown ? 0.0f : card.maxCooldown;
                selectedCard = -1;
            }
        }
    }
}

void Game::UpdatePlaying(float dt) {
    if (bannerTimer > 0.0f) bannerTimer -= dt;

    // 卡片冷却
    for (auto& c : cards) {
        if (c.cooldownTimer > 0.0f) {
            c.cooldownTimer = std::max(0.0f, c.cooldownTimer - (instantCooldown ? 100.0f : dt));
        }
    }

    // 天空自然掉落阳光
    skySunTimer -= dt;
    if (skySunTimer <= 0.0f) {
        SpawnSkySun();
        skySunTimer = RandRange(7.5f, 10.5f);
    }

    // 僵尸波次生成
    if (zombiesSpawned < totalZombiesInLevel) {
        spawnTimer -= dt * spawnRateMultiplier;
        if (spawnTimer <= 0.0f) {
            int row = GetRandomValue(0, GRID_ROWS - 1);
            int roll = GetRandomValue(1, 100);
            ZombieType zType = ZombieType::NORMAL;

            float progress = (difficultyIdx == 2)
                ? std::min(1.0f, zombiesSpawned / 40.0f)
                : static_cast<float>(zombiesSpawned) / static_cast<float>(totalZombiesInLevel);

            if (progress > 0.55f && roll > 70) {
                zType = ZombieType::BUCKETHEAD;
            } else if (progress > 0.2f && roll > 45) {
                zType = ZombieType::CONEHEAD;
            }

            SpawnZombie(zType, row);
            zombiesSpawned++;

            if (difficultyIdx != 2 && zombiesSpawned == totalZombiesInLevel - 5) {
                bannerText = "A HUGE WAVE OF ZOMBIES IS APPROACHING!";
                bannerTimer = 3.0f;
            } else if (difficultyIdx == 2 && zombiesSpawned % 12 == 0) {
                currentWave++;
                bannerText = TextFormat("ENDLESS WAVE %d!", currentWave);
                bannerTimer = 2.5f;
            }

            float baseInterval = std::max(1.2f, 5.2f - progress * 3.2f);
            if (difficultyIdx == 1) baseInterval *= 0.8f;
            spawnTimer = RandRange(baseInterval * 0.7f, baseInterval * 1.1f);
        }
    }

    // 更新植物逻辑
    for (auto& p : plants) {
        if (p.hp <= 0.0f) continue;
        if (p.hitFlashTimer > 0.0f) p.hitFlashTimer -= dt;

        float centerX = GRID_X + p.col * CELL_W + CELL_W * 0.5f;
        float centerY = GRID_Y + p.row * CELL_H + CELL_H * 0.5f;

        if (p.type == PlantType::SUNFLOWER) {
            p.actionTimer -= dt;
            if (p.actionTimer <= 0.0f) {
                SunOrb orb{};
                orb.x = centerX + RandRange(-18.0f, 18.0f);
                orb.y = centerY - 25.0f;
                orb.targetY = centerY + 15.0f;
                orb.value = 25;
                orb.lifeTimer = 10.0f;
                sunOrbs.push_back(orb);
                p.actionTimer = 15.0f;
            }
        } else if (p.type == PlantType::PEASHOOTER || p.type == PlantType::SNOWPEA) {
            bool zombieAhead = false;
            for (const auto& z : zombies) {
                if (z.hp > 0 && z.row == p.row && z.x > centerX - 10.0f && z.x < SCREEN_W + 10.0f) {
                    zombieAhead = true;
                    break;
                }
            }
            if (zombieAhead) {
                p.actionTimer -= dt;
                if (p.actionTimer <= 0.0f) {
                    Projectile proj{};
                    proj.type = (p.type == PlantType::SNOWPEA) ? ProjectileType::ICE_PEA : ProjectileType::PEA;
                    proj.row = p.row;
                    proj.x = centerX + 24.0f;
                    proj.y = centerY - 10.0f;
                    proj.speed = 380.0f;
                    proj.damage = 28.0f;
                    projectiles.push_back(proj);
                    p.actionTimer = 1.45f;
                }
            }
        } else if (p.type == PlantType::CHERRYBOMB) {
            p.actionTimer -= dt;
            if (p.actionTimer <= 0.0f) {
                for (auto& z : zombies) {
                    if (z.hp > 0 && std::abs(z.row - p.row) <= 1 &&
                        std::abs(z.x - centerX) <= CELL_W * 1.65f) {
                        z.hp = 0.0f;
                        score += 50;
                        SpawnParticles(z.x, GRID_Y + z.row * CELL_H + CELL_H * 0.5f, ORANGE, 12, 160.0f);
                    }
                }
                SpawnParticles(centerX, centerY, RED, 36, 240.0f);
                SpawnParticles(centerX, centerY, YELLOW, 24, 180.0f);
                p.hp = 0.0f;
            }
        }
    }

    // 更新子弹与命中判定
    for (auto& proj : projectiles) {
        if (!proj.alive) continue;
        proj.x += proj.speed * dt;
        if (proj.x > SCREEN_W + 40.0f) {
            proj.alive = false;
            continue;
        }

        for (auto& z : zombies) {
            if (z.hp > 0 && z.row == proj.row && std::abs(proj.x - z.x) < 28.0f) {
                z.hp -= proj.damage;
                z.hitFlashTimer = 0.12f;
                if (proj.type == ProjectileType::ICE_PEA) {
                    z.slowTimer = 4.0f;
                    SpawnParticles(proj.x, proj.y, SKYBLUE, 6, 70.0f);
                } else {
                    SpawnParticles(proj.x, proj.y, LIME, 6, 70.0f);
                }
                if (z.hp <= 0.0f) {
                    score += (z.type == ZombieType::BUCKETHEAD) ? 100 : (z.type == ZombieType::CONEHEAD ? 60 : 40);
                }
                proj.alive = false;
                break;
            }
        }
    }

    // 更新僵尸逻辑
    for (auto& z : zombies) {
        if (z.hp <= 0.0f) continue;
        if (z.hitFlashTimer > 0.0f) z.hitFlashTimer -= dt;
        if (z.slowTimer > 0.0f) z.slowTimer -= dt;

        float speedScale = (z.slowTimer > 0.0f) ? 0.5f : 1.0f;
        z.animPhase += dt * 4.5f * speedScale;

        // 检查是否碰到同行的植物
        Plant* targetPlant = nullptr;
        int col = static_cast<int>((z.x - GRID_X) / CELL_W);
        if (col >= 0 && col < GRID_COLS) {
            Plant* p = GetPlantAt(z.row, col);
            if (p != nullptr && p->type != PlantType::CHERRYBOMB) {
                float plantX = GRID_X + col * CELL_W + CELL_W * 0.5f;
                if (std::abs(z.x - plantX) < 38.0f) {
                    targetPlant = p;
                }
            }
        }

        if (targetPlant != nullptr) {
            z.isEating = true;
            targetPlant->hp -= z.dps * speedScale * dt;
            targetPlant->hitFlashTimer = 0.08f;
        } else {
            z.isEating = false;
            z.x -= z.speed * speedScale * dt;
        }

        // 检查小推车或失败条件
        auto& mower = mowers[z.row];
        if (mower.active && !mower.triggered && z.x <= mower.x + 28.0f) {
            mower.triggered = true;
            bannerText = "LAWN MOWER ACTIVATED!";
            bannerTimer = 1.8f;
        }
        if (z.x < GRID_X - 80.0f) {
            state = GameState::GAME_OVER;
        }
    }

    // 更新小推车
    for (auto& m : mowers) {
        if (!m.active || !m.triggered) continue;
        m.x += 540.0f * dt;
        for (auto& z : zombies) {
            if (z.hp > 0 && z.row == m.row && std::abs(z.x - m.x) < 42.0f) {
                z.hp = 0.0f;
                score += 30;
                SpawnParticles(z.x, GRID_Y + z.row * CELL_H + CELL_H * 0.5f, GRAY, 12, 130.0f);
            }
        }
        if (m.x > SCREEN_W + 80.0f) m.active = false;
    }

    // 更新阳光球
    for (auto& orb : sunOrbs) {
        if (orb.collected) continue;
        if (orb.y < orb.targetY) {
            orb.y = std::min(orb.targetY, orb.y + 75.0f * dt);
        }
        orb.lifeTimer -= dt;
        if (orb.lifeTimer <= 0.0f) orb.collected = true;
    }

    // 更新粒子
    for (auto& pt : particles) {
        pt.x += pt.vx * dt;
        pt.y += pt.vy * dt;
        pt.life -= dt;
    }

    // 清理失效实体
    plants.erase(std::remove_if(plants.begin(), plants.end(), [](const Plant& p) { return p.hp <= 0.0f; }), plants.end());
    zombies.erase(std::remove_if(zombies.begin(), zombies.end(), [](const Zombie& z) { return z.hp <= 0.0f; }), zombies.end());
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& pr) { return !pr.alive; }), projectiles.end());
    sunOrbs.erase(std::remove_if(sunOrbs.begin(), sunOrbs.end(), [](const SunOrb& o) { return o.collected; }), sunOrbs.end());
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& pt) { return pt.life <= 0.0f; }), particles.end());

    // 检查胜利条件
    if (difficultyIdx != 2 && zombiesSpawned >= totalZombiesInLevel && zombies.empty()) {
        state = GameState::VICTORY;
    }
}

void Game::Draw() {
    BeginDrawing();
    ClearBackground({45, 75, 45, 255});

    DrawLawn();

    if (state == GameState::MENU) {
        DrawMenuGUI();
        if (showAlmanac) DrawAlmanacWindow();
        EndDrawing();
        return;
    }

    DrawMowers();
    DrawPlants();
    DrawZombies();
    DrawProjectiles();
    DrawParticles();
    DrawPlacementPreview();
    DrawSunOrbs();
    DrawTopHUD();

    // 波次提示横幅
    if (bannerTimer > 0.0f) {
        int tw = MeasureText(bannerText.c_str(), 32);
        DrawRectangle(SCREEN_W / 2 - tw / 2 - 24, SCREEN_H / 2 - 35, tw + 48, 60, {0, 0, 0, 165});
        DrawText(bannerText.c_str(), SCREEN_W / 2 - tw / 2, SCREEN_H / 2 - 20, 32, {255, 80, 80, 255});
    }

    if (showDevGui) {
        DrawDevGuiWindow();
    }

    if (state == GameState::PLAYING && isPaused) {
        DrawPauseOrEndGUI();
    } else if (state == GameState::VICTORY || state == GameState::GAME_OVER) {
        DrawPauseOrEndGUI();
    }

    EndDrawing();
}

void Game::DrawLawn() {
    // 顶部木质/深色 HUD 背景区
    DrawRectangle(0, 0, SCREEN_W, static_cast<int>(GRID_Y) - 10, {72, 48, 30, 255});
    DrawRectangle(0, static_cast<int>(GRID_Y) - 10, SCREEN_W, 10, {48, 30, 18, 255});

    // 左侧房屋安全区与右侧僵尸路径边界
    DrawRectangle(0, static_cast<int>(GRID_Y), static_cast<int>(GRID_X), static_cast<int>(GRID_ROWS * CELL_H), {115, 105, 90, 255});
    DrawRectangle(static_cast<int>(GRID_X + GRID_COLS * CELL_W), static_cast<int>(GRID_Y),
                  SCREEN_W - static_cast<int>(GRID_X + GRID_COLS * CELL_W), static_cast<int>(GRID_ROWS * CELL_H), {90, 120, 65, 255});

    // 5x9 棋盘草坪
    for (int r = 0; r < GRID_ROWS; ++r) {
        for (int c = 0; c < GRID_COLS; ++c) {
            Color tileColor = ((r + c) % 2 == 0) ? Color{85, 175, 70, 255} : Color{72, 155, 58, 255};
            DrawRectangle(static_cast<int>(GRID_X + c * CELL_W),
                          static_cast<int>(GRID_Y + r * CELL_H),
                          static_cast<int>(CELL_W),
                          static_cast<int>(CELL_H),
                          tileColor);
        }
    }

    // 底部状态提示
    DrawText("Controls: [1-5] Select Plant | [S] Shovel | [Right Click] Cancel | [P / ESC] Pause",
             20, SCREEN_H - 32, 18, {220, 235, 210, 255});
    DrawText(TextFormat("Score: %d", score), SCREEN_W - 180, SCREEN_H - 34, 22, GOLD);
}

void Game::DrawMowers() {
    for (const auto& m : mowers) {
        if (!m.active) continue;
        float cy = GRID_Y + m.row * CELL_H + CELL_H * 0.5f;
        DrawRectangleRounded({m.x - 24, cy - 18, 48, 32}, 0.3f, 6, RED);
        DrawRectangle(static_cast<int>(m.x - 10), static_cast<int>(cy - 26), 20, 12, LIGHTGRAY);
        DrawCircle(static_cast<int>(m.x - 14), static_cast<int>(cy + 16), 9, DARKGRAY);
        DrawCircle(static_cast<int>(m.x + 14), static_cast<int>(cy + 16), 9, DARKGRAY);
    }
}

void Game::DrawPlants() {
    for (const auto& p : plants) {
        float cx = GRID_X + p.col * CELL_W + CELL_W * 0.5f;
        float cy = GRID_Y + p.row * CELL_H + CELL_H * 0.5f;
        Color tint = (p.hitFlashTimer > 0.0f) ? Color{255, 220, 220, 255} : WHITE;

        // 地面阴影
        DrawEllipse(static_cast<int>(cx), static_cast<int>(cy + 34), 28, 10, {0, 0, 0, 70});

        switch (p.type) {
            case PlantType::SUNFLOWER: {
                DrawRectangle(static_cast<int>(cx - 4), static_cast<int>(cy + 6), 8, 28, {40, 140, 40, 255});
                for (int i = 0; i < 8; ++i) {
                    float ang = i * 0.7854f + static_cast<float>(GetTime()) * 0.8f;
                    DrawCircle(static_cast<int>(cx + std::cos(ang) * 20),
                               static_cast<int>(cy - 6 + std::sin(ang) * 20), 10, GOLD);
                }
                DrawCircle(static_cast<int>(cx), static_cast<int>(cy - 6), 17, {135, 85, 30, 255});
                DrawCircle(static_cast<int>(cx - 5), static_cast<int>(cy - 9), 2, BLACK);
                DrawCircle(static_cast<int>(cx + 5), static_cast<int>(cy - 9), 2, BLACK);
                break;
            }
            case PlantType::PEASHOOTER:
            case PlantType::SNOWPEA: {
                Color headCol = (p.type == PlantType::SNOWPEA) ? Color{85, 200, 240, 255} : Color{80, 200, 70, 255};
                if (p.hitFlashTimer > 0.0f) headCol = tint;
                DrawRectangle(static_cast<int>(cx - 4), static_cast<int>(cy + 4), 8, 30, {40, 140, 40, 255});
                DrawCircle(static_cast<int>(cx), static_cast<int>(cy - 8), 21, headCol);
                DrawRectangle(static_cast<int>(cx + 12), static_cast<int>(cy - 17), 20, 15, headCol);
                DrawCircle(static_cast<int>(cx + 32), static_cast<int>(cy - 10), 6, DARKGREEN);
                DrawCircle(static_cast<int>(cx + 6), static_cast<int>(cy - 14), 4, WHITE);
                DrawCircle(static_cast<int>(cx + 7), static_cast<int>(cy - 14), 2, BLACK);
                break;
            }
            case PlantType::WALLNUT: {
                Color nutCol = {175, 120, 65, 255};
                DrawEllipse(static_cast<int>(cx), static_cast<int>(cy + 2), 26, 32, nutCol);
                DrawCircle(static_cast<int>(cx - 8), static_cast<int>(cy - 8), 4, WHITE);
                DrawCircle(static_cast<int>(cx + 8), static_cast<int>(cy - 8), 4, WHITE);
                DrawCircle(static_cast<int>(cx - 7), static_cast<int>(cy - 8), 2, BLACK);
                DrawCircle(static_cast<int>(cx + 9), static_cast<int>(cy - 8), 2, BLACK);
                if (p.hp < p.maxHp * 0.65f) {
                    DrawLine(static_cast<int>(cx - 12), static_cast<int>(cy + 5), static_cast<int>(cx + 8), static_cast<int>(cy + 14), DARKBROWN);
                }
                if (p.hp < p.maxHp * 0.35f) {
                    DrawLine(static_cast<int>(cx - 6), static_cast<int>(cy - 20), static_cast<int>(cx + 4), static_cast<int>(cy), DARKBROWN);
                }
                break;
            }
            case PlantType::CHERRYBOMB: {
                float pulse = 1.0f + 0.12f * std::sin(static_cast<float>(GetTime()) * 20.0f);
                DrawCircle(static_cast<int>(cx - 12), static_cast<int>(cy + 10), 16 * pulse, RED);
                DrawCircle(static_cast<int>(cx + 12), static_cast<int>(cy + 12), 16 * pulse, MAROON);
                DrawLine(static_cast<int>(cx - 12), static_cast<int>(cy - 4), static_cast<int>(cx), static_cast<int>(cy - 26), GREEN);
                DrawLine(static_cast<int>(cx + 12), static_cast<int>(cy - 2), static_cast<int>(cx), static_cast<int>(cy - 26), GREEN);
                DrawCircle(static_cast<int>(cx), static_cast<int>(cy - 28), 5, YELLOW);
                break;
            }
            default:
                break;
        }

        if (showDebugInfo || p.hp < p.maxHp) {
            float ratio = std::max(0.0f, p.hp / p.maxHp);
            DrawRectangle(static_cast<int>(cx - 26), static_cast<int>(cy + 40), 52, 5, DARKGRAY);
            DrawRectangle(static_cast<int>(cx - 26), static_cast<int>(cy + 40), static_cast<int>(52 * ratio), 5, GREEN);
        }
    }
}

void Game::DrawZombies() {
    for (const auto& z : zombies) {
        float cy = GRID_Y + z.row * CELL_H + CELL_H * 0.5f;
        float bob = std::sin(z.animPhase) * 3.0f;

        Color skin = (z.slowTimer > 0.0f) ? Color{120, 210, 255, 255} : Color{135, 165, 120, 255};
        Color coat = (z.slowTimer > 0.0f) ? Color{85, 130, 185, 255} : Color{95, 75, 65, 255};
        if (z.hitFlashTimer > 0.0f) {
            skin = WHITE;
            coat = LIGHTGRAY;
        }

        DrawEllipse(static_cast<int>(z.x), static_cast<int>(cy + 36), 24, 9, {0, 0, 0, 75});

        // 躯干与手臂
        DrawRectangle(static_cast<int>(z.x - 13), static_cast<int>(cy - 10 + bob), 26, 36, coat);
        DrawRectangle(static_cast<int>(z.x - 24), static_cast<int>(cy - 6 + bob), 18, 8, skin);
        // 腿
        DrawRectangle(static_cast<int>(z.x - 10), static_cast<int>(cy + 26), 8, 12, DARKBROWN);
        DrawRectangle(static_cast<int>(z.x + 2), static_cast<int>(cy + 26), 8, 12, DARKBROWN);
        // 头
        DrawCircle(static_cast<int>(z.x), static_cast<int>(cy - 22 + bob), 16, skin);
        DrawCircle(static_cast<int>(z.x - 6), static_cast<int>(cy - 24 + bob), 3, RED);

        // 护具可视化（血量低于阈值后护具脱落）
        if (z.type == ZombieType::CONEHEAD && z.hp > 200.0f) {
            DrawTriangle({z.x, cy - 58 + bob}, {z.x - 15, cy - 32 + bob}, {z.x + 15, cy - 32 + bob}, ORANGE);
        } else if (z.type == ZombieType::BUCKETHEAD && z.hp > 200.0f) {
            DrawRectangle(static_cast<int>(z.x - 15), static_cast<int>(cy - 54 + bob), 30, 24, GRAY);
            DrawRectangleLines(static_cast<int>(z.x - 15), static_cast<int>(cy - 54 + bob), 30, 24, DARKGRAY);
        }

        // 血条
        if (showDebugInfo || z.hp < z.maxHp) {
            float ratio = std::max(0.0f, z.hp / z.maxHp);
            DrawRectangle(static_cast<int>(z.x - 24), static_cast<int>(cy - 64 + bob), 48, 5, DARKGRAY);
            DrawRectangle(static_cast<int>(z.x - 24), static_cast<int>(cy - 64 + bob), static_cast<int>(48 * ratio), 5, RED);
        }
    }
}

void Game::DrawProjectiles() {
    for (const auto& pr : projectiles) {
        Color col = (pr.type == ProjectileType::ICE_PEA) ? Color{110, 230, 255, 255} : Color{110, 235, 70, 255};
        DrawCircle(static_cast<int>(pr.x), static_cast<int>(pr.y), 9, col);
        DrawCircle(static_cast<int>(pr.x - 3), static_cast<int>(pr.y - 3), 3, WHITE);
    }
}

void Game::DrawSunOrbs() {
    for (const auto& orb : sunOrbs) {
        float pulse = 1.0f + 0.08f * std::sin(static_cast<float>(GetTime()) * 6.0f);
        DrawCircle(static_cast<int>(orb.x), static_cast<int>(orb.y), 24 * pulse, {255, 220, 50, 140});
        DrawCircle(static_cast<int>(orb.x), static_cast<int>(orb.y), 18 * pulse, YELLOW);
        DrawText("+25", static_cast<int>(orb.x - 12), static_cast<int>(orb.y - 7), 14, ORANGE);
    }
}

void Game::DrawParticles() {
    for (const auto& pt : particles) {
        float alpha = std::max(0.0f, pt.life / pt.maxLife);
        Color c = pt.color;
        c.a = static_cast<unsigned char>(255 * alpha);
        DrawCircle(static_cast<int>(pt.x), static_cast<int>(pt.y), pt.radius, c);
    }
}

void Game::DrawPlacementPreview() {
    Vector2 mouse = GetMousePosition();
    if (mouse.x < GRID_X || mouse.x >= GRID_X + GRID_COLS * CELL_W ||
        mouse.y < GRID_Y || mouse.y >= GRID_Y + GRID_ROWS * CELL_H) {
        return;
    }

    int col = static_cast<int>((mouse.x - GRID_X) / CELL_W);
    int row = static_cast<int>((mouse.y - GRID_Y) / CELL_H);
    Rectangle cellRec = {GRID_X + col * CELL_W, GRID_Y + row * CELL_H, CELL_W, CELL_H};

    if (shovelSelected) {
        Color hl = IsCellOccupied(row, col) ? Color{255, 80, 80, 100} : Color{255, 255, 255, 45};
        DrawRectangleRec(cellRec, hl);
        DrawRectangleLinesEx(cellRec, 2.0f, RED);
    } else if (selectedCard >= 0) {
        Color hl = IsCellOccupied(row, col) ? Color{255, 60, 60, 100} : Color{180, 255, 140, 100};
        DrawRectangleRec(cellRec, hl);
        DrawRectangleLinesEx(cellRec, 2.0f, WHITE);
    }
}

void Game::DrawTopHUD() {
    // 左侧阳光计数器
    DrawRectangleRounded({18, 16, 108, 92}, 0.15f, 6, {245, 225, 165, 255});
    DrawRectangleRoundedLinesEx({18, 16, 108, 92}, 0.15f, 6, 3.0f, {140, 95, 45, 255});
    DrawCircle(72, 46, 18, GOLD);
    DrawCircle(72, 46, 12, YELLOW);
    std::string sunTxt = std::to_string(sun);
    int tw = MeasureText(sunTxt.c_str(), 26);
    DrawText(sunTxt.c_str(), 72 - tw / 2, 74, 26, BLACK);

    // 植物种子卡片栏
    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        const auto& c = cards[i];
        Rectangle rec = {140.0f + i * 96.0f, 14.0f, 88.0f, 96.0f};
        bool affordable = (sun >= c.cost);
        bool ready = (c.cooldownTimer <= 0.0f);

        Color bg = (affordable && ready) ? c.cardColor : Color{115, 115, 115, 255};
        DrawRectangleRounded(rec, 0.12f, 6, bg);
        DrawRectangleRoundedLinesEx(rec, 0.12f, 6, (selectedCard == i) ? 4.0f : 2.0f,
                                    (selectedCard == i) ? YELLOW : DARKBROWN);

        DrawText(TextFormat("[%d]", i + 1), static_cast<int>(rec.x + 6), static_cast<int>(rec.y + 6), 14, DARKGRAY);
        DrawText(c.name.c_str(), static_cast<int>(rec.x + 6), static_cast<int>(rec.y + 38), 14, BLACK);
        DrawText(TextFormat("%d", c.cost), static_cast<int>(rec.x + 8), static_cast<int>(rec.y + 72), 20,
                 affordable ? BLACK : RED);

        if (c.cooldownTimer > 0.0f) {
            float ratio = c.cooldownTimer / c.maxCooldown;
            DrawRectangle(static_cast<int>(rec.x), static_cast<int>(rec.y),
                          static_cast<int>(rec.width), static_cast<int>(rec.height * ratio), {0, 0, 0, 150});
        }
    }

    // 铲子槽位
    Rectangle shovelRec = {140.0f + cards.size() * 96.0f + 10.0f, 22.0f, 76.0f, 80.0f};
    DrawRectangleRounded(shovelRec, 0.15f, 6, shovelSelected ? ORANGE : Color{180, 175, 165, 255});
    DrawRectangleRoundedLinesEx(shovelRec, 0.15f, 6, shovelSelected ? 3.5f : 2.0f, DARKBROWN);
    DrawText("SHOVEL", static_cast<int>(shovelRec.x + 10), static_cast<int>(shovelRec.y + 24), 14, BLACK);
    DrawText("[S]", static_cast<int>(shovelRec.x + 26), static_cast<int>(shovelRec.y + 48), 16, DARKGRAY);

    // 右上角 Raygui 控制栏：进度条 + 倍速 + 调试面板 + 暂停
    float progress = (difficultyIdx == 2)
        ? std::min(1.0f, (zombiesSpawned % 12) / 12.0f)
        : static_cast<float>(zombiesSpawned) / static_cast<float>(totalZombiesInLevel);

    GuiLabel({740, 14, 220, 22}, (difficultyIdx == 2)
        ? TextFormat("Endless Wave %d", currentWave)
        : TextFormat("Wave Progress: %d / %d", zombiesSpawned, totalZombiesInLevel));
    GuiProgressBar({740, 38, 250, 24}, "", "", &progress, 0.0f, 1.0f);

    GuiToggle({740, 74, 115, 34}, speed2x ? "Speed: 2x" : "Speed: 1x", &speed2x);
    GuiToggle({865, 74, 125, 34}, "Dev GUI", &showDevGui);

    if (GuiButton({1015, 22, 115, 38}, isPaused ? "Resume" : "Pause")) {
        isPaused = !isPaused;
    }
    if (GuiButton({1145, 22, 115, 38}, "Menu")) {
        state = GameState::MENU;
    }
    if (GuiButton({1015, 70, 245, 38}, "Restart Level")) {
        StartNewGame();
    }
}

void Game::DrawMenuGUI() {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, {0, 0, 0, 130});

    Rectangle panel = {SCREEN_W / 2.0f - 230.0f, 130.0f, 460.0f, 440.0f};
    GuiPanel(panel, "PLANTS VS ZOMBIES - RAYLIB EDITION");

    DrawText("PLANTS VS ZOMBIES", static_cast<int>(panel.x + 65), static_cast<int>(panel.y + 52), 30, DARKGREEN);
    DrawText("Built with Raylib + Raygui + MSVC + vcpkg", static_cast<int>(panel.x + 68), static_cast<int>(panel.y + 92), 16, DARKGRAY);

    GuiLabel({panel.x + 60, panel.y + 140, 130, 32}, "Game Mode:");
    GuiComboBox({panel.x + 195, panel.y + 140, 205, 34}, "Normal;Hard;Endless", &difficultyIdx);

    if (GuiButton({panel.x + 60, panel.y + 205, 340, 52}, "START ADVENTURE")) {
        StartNewGame();
    }
    if (GuiButton({panel.x + 60, panel.y + 275, 340, 46}, "ALMANAC & GUIDE")) {
        showAlmanac = true;
    }
    if (GuiButton({panel.x + 60, panel.y + 340, 340, 46}, "EXIT GAME")) {
        shouldQuit = true;
    }
}

void Game::DrawAlmanacWindow() {
    Rectangle win = {SCREEN_W / 2.0f - 300.0f, 160.0f, 600.0f, 400.0f};
    if (GuiWindowBox(win, "Plant Almanac & How to Play")) {
        showAlmanac = false;
    }

    int y = static_cast<int>(win.y) + 45;
    int x = static_cast<int>(win.x) + 24;
    DrawText("1. Sunflower (50 Sun)    - Produces extra sun every 15s.", x, y, 18, DARKBROWN);
    DrawText("2. Peashooter (100 Sun)  - Shoots peas down the lane.", x, y + 38, 18, DARKGREEN);
    DrawText("3. Wall-nut (50 Sun)     - High HP defensive barrier.", x, y + 76, 18, BROWN);
    DrawText("4. Snow Pea (175 Sun)    - Shoots ice peas that slow zombies.", x, y + 114, 18, DARKBLUE);
    DrawText("5. Cherry Bomb (150 Sun) - Explodes 3x3 area instantly!", x, y + 152, 18, RED);
    DrawText("Tips: Click falling sun orbs to collect (+25 Sun).", x, y + 210, 18, BLACK);
    DrawText("      Lawn mowers clear a row once if zombies break through.", x, y + 242, 18, BLACK);

    if (GuiButton({win.x + win.width / 2.0f - 70.0f, win.y + win.height - 55.0f, 140.0f, 38.0f}, "CLOSE")) {
        showAlmanac = false;
    }
}

void Game::DrawDevGuiWindow() {
    Rectangle win = GetDevWindowRect();
    if (GuiWindowBox(win, "Raygui Dev / Sandbox Panel")) {
        showDevGui = false;
    }

    float x = win.x + 16.0f;
    float y = win.y + 38.0f;
    float w = win.width - 32.0f;

    if (GuiButton({x, y, w, 32}, "+100 Sun")) {
        sun += 100;
    }
    if (GuiButton({x, y + 40, w, 32}, "Spawn Zombie Wave")) {
        for (int r = 0; r < GRID_ROWS; ++r) {
            SpawnZombie(static_cast<ZombieType>(GetRandomValue(0, 2)), r);
        }
    }
    GuiCheckBox({x, y + 86, 22, 22}, "Show HP & Debug Bars", &showDebugInfo);
    GuiCheckBox({x, y + 118, 22, 22}, "Instant Plant Cooldown", &instantCooldown);

    GuiLabel({x, y + 154, w, 22}, TextFormat("Spawn Rate Multiplier: %.1fx", spawnRateMultiplier));
    GuiSlider({x, y + 180, w, 24}, "0.5x", "3.0x", &spawnRateMultiplier, 0.5f, 3.0f);
}

void Game::DrawPauseOrEndGUI() {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, {0, 0, 0, 150});
    Rectangle box = {SCREEN_W / 2.0f - 200.0f, SCREEN_H / 2.0f - 150.0f, 400.0f, 290.0f};

    const char* title = "GAME PAUSED";
    if (state == GameState::VICTORY) title = "VICTORY! YOU DEFENDED THE LAWN!";
    if (state == GameState::GAME_OVER) title = "GAME OVER - ZOMBIES ATE YOUR BRAINS!";

    GuiPanel(box, title);

    DrawText(TextFormat("Final Score: %d", score), static_cast<int>(box.x + 115), static_cast<int>(box.y + 58), 22, DARKBLUE);

    float btnY = box.y + 105.0f;
    if (state == GameState::PLAYING) {
        if (GuiButton({box.x + 60, btnY, 280, 44}, "RESUME")) {
            isPaused = false;
        }
        btnY += 56.0f;
    }
    if (GuiButton({box.x + 60, btnY, 280, 44}, "PLAY AGAIN")) {
        StartNewGame();
    }
    if (GuiButton({box.x + 60, btnY + 56.0f, 280, 44}, "BACK TO MENU")) {
        state = GameState::MENU;
        isPaused = false;
    }
}

} // namespace pvz
