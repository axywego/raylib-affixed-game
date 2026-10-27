#pragma once

#include <raylib.h>
#include <vector>
#include <memory>

#include "../entity/entity.hpp"
#include "../../core/input/input.hpp"
#include "../render/renderer.hpp"
#include "../player/player.hpp"
#include "game/enemy/crowd.hpp"
#include "game/spell_system/spell_system.hpp"
#include "game/world/world.hpp"

namespace game {
    class Scene {
    public:
        void setup();
        void handleInput();
        void update(float dt);
        void animate(float dt);
        void render(float alpha);
        void shutdown();

        bool debug_ {true};
    private:
        input::InputMap inputMap_;
        Renderer renderer_;
        Camera3D camera_;

        Player* player_{};

        SpellSystem spellSystem_;
        std::vector<std::unique_ptr<entity::Entity>> entities_;
        std::vector<entity::enemy::Enemy*> enemies_;
        entity::enemy::EnemyCrowd crowd_;

        bool camOrbit_ {false};
        float camYaw_ {0.0f};
        float camPitch_ {0.55f};
        float camDist_ {22.0f};

        world::World world_;
    };
}
