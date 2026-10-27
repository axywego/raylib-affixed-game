#include "core/config/config.hpp"
#include "game/scene/scene.hpp"
#include <raylib.h>

int main() {
    cfg::setup();

    game::Scene scene;
    scene.setup();

    double accumulator = .0;
    while(!WindowShouldClose()) {
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();
        accumulator += GetFrameTime();

        if (accumulator > 0.25) accumulator = 0.25;

        scene.handleInput();

        while(accumulator >= cfg::kTickDt) {
            scene.update(static_cast<float>(cfg::kTickDt));
            accumulator -= cfg::kTickDt;
        }

        scene.animate(GetFrameTime());
        const float alpha = static_cast<float>(accumulator / cfg::kTickDt);

        BeginDrawing();
        ClearBackground(Color{20, 20, 28, 255});

        scene.render(alpha);
        DrawFPS(10, 10);

        DrawText("WASD - move | Shift - jog | Alt - sprint | F11 - fullscreen | Esc - exit", 10,36,20,RAYWHITE);
        EndDrawing();
    }
    scene.shutdown();
    CloseWindow();

    return 0;
}
