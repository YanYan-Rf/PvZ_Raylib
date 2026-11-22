#include "Game.hpp"
#include <raylib.h>

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(pvz::SCREEN_W, pvz::SCREEN_H, "Plants vs. Zombies (Raylib + Raygui + MSVC)");
    SetExitKey(KEY_NULL); // 交给游戏内 ESC 处理暂停，不直接退出窗口
    SetTargetFPS(60);

    pvz::Game game;

    while (!WindowShouldClose() && !game.ShouldQuit()) {
        float dt = GetFrameTime();
        // 避免切窗口或断点恢复时出现超大 dt 导致穿模
        if (dt > 0.05f) dt = 0.05f;

        game.Update(dt);
        game.Draw();
    }

    CloseWindow();
    return 0;
}
