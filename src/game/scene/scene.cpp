#include "scene.hpp"
#include <memory>
#include <raylib.h>
#include <raymath.h>
#include <cmath>

#include "../../core/projection.hpp"
#include "core/input/input.hpp"
#include "game/enemy/enemy.hpp"
#include "game/spell_system/spell_system.hpp"

void game::Scene::setup() {
    camera_.up         = {0.0f, 1.0f, 0.0f};
    camera_.fovy       = 40.0f;
    camera_.projection = CAMERA_PERSPECTIVE;

    camYaw_   = atan2f(13.0f, 0.0f);
    camPitch_ = 0.55f;
    camDist_  = 22.0f;

    world_.init(32, 32, 1.f);

    for (int x = 0; x < 32; ++x) {
        world_.setTile(x, 0, world::TileType::Wall);
        world_.setTile(x, 31, world::TileType::Wall);
    }
    for (int y = 1; y < 31; ++y) {
        world_.setTile(0, y, world::TileType::Wall);
        world_.setTile(31, y, world::TileType::Wall);
        for (int x = 1; x < 31; ++x) {
            world_.setTile(x, y, world::TileType::Floor);
        }
    }

    for(int y = 1; y < 31; ++y) {
        if(y != 16 && y != 15) world_.setTile(16, y, world::TileType::Wall);

    }

    entities_.push_back(std::make_unique<Player>());
    player_ = static_cast<Player*>(entities_.back().get());
    player_->transform_.scale = {1.5f, 1.5f, 1.5f};
    player_->transform_.position = {14.5f, 0.f, 30.f};

    auto spawnEnemy = [&](Vector3 pos) {
        auto enemy = std::make_unique<entity::enemy::Enemy>(pos, player_);
        enemies_.push_back(enemy.get());
        crowd_.add(enemy.get());
        entities_.push_back(std::move(enemy));
    };

    // for (int row = 0; row < 4; ++row) {
    //     for (int col = 0; col < 6; ++col) {
    //         spawnEnemy({18.5f + col * 2.0f, 0.5f, 9.5f + row * 2.0f});
    //     }
    // }

    const Vector3 leftSpawns[] = {
        {9.5f, 0.5f, 25.5f}, {11.5f, 0.5f, 25.5f}, {13.5f, 0.5f, 25.5f},
        {9.5f, 0.5f, 27.5f}, {11.5f, 0.5f, 27.5f}, {13.5f, 0.5f, 27.5f},
        {10.5f, 0.5f, 29.5f}, {12.5f, 0.5f, 29.5f},
    };

    for (const Vector3& pos : leftSpawns) {
        spawnEnemy(pos);
    }

    spellSystem_.setSlot(0, &kFireball);
    spellSystem_.setSlot(1, &kFirezone);

    // for(int i = 0; i < 100; ++i) {
    //     for(int j = 0; j < 100; ++j) {
    //         world_.addBox({-5.f, 0.5f + i * 1, -40.f + j * 1.f}, {1.f, 1.f, 1.f}, BLUE);
    //     }
    // }



}

void game::Scene::handleInput() {
    if (IsKeyPressed(KEY_F3)) debug_ = !debug_;

    const bool hasMove = Vector2LengthSqr(input::ReadMoveInput(inputMap_)) > 0.0f;
    if (hasMove) {
        if (inputMap_.isDown(input::Action::Sprint))
            player_->setMoveMode(Player::MoveMode::Sprint);
        else if (inputMap_.isDown(input::Action::Jog))
            player_->setMoveMode(Player::MoveMode::Jog);
        else
            player_->setMoveMode(Player::MoveMode::Walk);
    }

    if (IsKeyPressed(KEY_RIGHT_BRACKET)) player_->adjustAnimTrim(+0.1f);
    if (IsKeyPressed(KEY_LEFT_BRACKET))  player_->adjustAnimTrim(-0.1f);

    int key = 0;
    while ((key = GetKeyPressed()) != 0) {
        if (key == KEY_CAPS_LOCK) {
            camOrbit_ = !camOrbit_;
            if (camOrbit_) DisableCursor();
            else EnableCursor();
        }
    }

    if (camOrbit_) {
        Vector2 delta = GetMouseDelta();
        camYaw_   += delta.x * 0.005f;
        camPitch_ -= delta.y * 0.004f;
        camPitch_  = Clamp(camPitch_, 0.15f, 1.2f);
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        camDist_ -= wheel * 2.0f;
        camDist_  = Clamp(camDist_, 10.0f, 36.0f);
    }

    Vector2 cursor{};
    const bool hasCursor = MouseToGround(camera_, cursor);

    if (inputMap_.isDown(input::Action::FirstSpell) && hasCursor) {
        Vector3 spawn = player_->transform_.position;
        spawn.y = 1;
        Vector2 dir = Vector2Subtract(cursor, {spawn.x, spawn.z});
        if (Vector2LengthSqr(dir) > 0.01f)
            spellSystem_.cast(0, spawn, dir);
    }

    if (inputMap_.isDown(input::Action::SecondSpell) && hasCursor) {
        Vector3 spawn = {cursor.x, 0.f, cursor.y};
        spellSystem_.castAtPoint(1, spawn);
    }

    player_->setMoveDir(input::ReadMoveInput(inputMap_));
}

void game::Scene::update(float dt) {
    spellSystem_.update(dt, world_);

    if (player_) {
        player_->update(dt, world_);
        const Vector2 playerDir = player_->velocityDir();
        crowd_.update(dt, world_, player_->transform_.position, player_->colliderSize_.x * 0.5f,
                      {playerDir.x, 0.0f, playerDir.y});
    }
}

void game::Scene::animate(float dt) {
    for(auto& e : entities_) {
        e->animate(dt);
    }
}

void game::Scene::render(float alpha) {
    if(!player_) return;

    Vector3 target = player_->renderPosition(alpha);

    const float cp = cosf(camPitch_);
    const float sp = sinf(camPitch_);
    const float cy = cosf(camYaw_);
    const float sy = sinf(camYaw_);

    camera_.target   = target;
    camera_.position.x = target.x + cy * cp * camDist_;
    camera_.position.y = target.y + sp * camDist_;
    camera_.position.z = target.z + sy * cp * camDist_;

    Vector2 cursor{};
    const bool hasCursor = MouseToGround(camera_, cursor);

    player_->updateFacing(cursor, hasCursor);

    BeginMode3D(camera_);
        world_.draw();

        for(const auto& e : entities_) {
            renderer_.draw(*e, { e->rotationDeg(), alpha });
        }
        spellSystem_.draw(alpha);
        if (hasCursor) {
            DrawSphere(ToWorld3D(cursor, 0.1f), 0.2f, ORANGE);
        }
    EndMode3D();

    if (debug_) {
        constexpr int fontSize = 22;
        constexpr int lh = fontSize + 6;
        constexpr int pad = 10;
        const int x = GetScreenWidth() - 380;
        const int y = 10;

        constexpr int lineCount = 9;

        DrawRectangle(x - pad, y - pad, 370, lh * lineCount + pad * 2, Color{0, 0, 0, 190});

        const char* lines[lineCount] = {
            TextFormat("spell 1 cooldown left:      %.3f", spellSystem_.getCooldownLeft(0)),
            TextFormat("spell 2 cooldown left:      %.3f", spellSystem_.getCooldownLeft(1)),
            TextFormat("spell 3 cooldown left:      %.3f", spellSystem_.getCooldownLeft(2)),
            TextFormat("spell 4 cooldown left:      %.3f", spellSystem_.getCooldownLeft(3)),
            // TextFormat("clip:      %s", player_->debugClipName()),
            // TextFormat("keyframes: %d", player_->debugKeyframes()),
            // TextFormat("frame:     %.2f", player_->debugFrame()),
            // TextFormat("mode:      %s", player_->debugModeName()),
            // TextFormat("spd:       %.3f  (target %.2f)", player_->debugSpeed(), player_->speed()),
            // TextFormat("clipSpd:   %.3f", player_->debugClipSpeed()),
            // TextFormat("ratio:     %.3f", player_->debugRatio()),
            // TextFormat("animTrim:  %.2f  [ ] keys", player_->animTrim()),
            // TextFormat("blend:     %.3f", player_->debugBlend()),
        };
        for (int i = 0; i < lineCount; ++i) {
            const Color c = (i == 0) ? YELLOW : RAYWHITE;
            DrawText(lines[i], x + 1, y + lh * i + 1, fontSize, Color{0, 0, 0, 220});
            DrawText(lines[i], x, y + lh * i, fontSize, c);
        }
    }
}

void game::Scene::shutdown() {
    crowd_.clear();
    enemies_.clear();
    player_ = nullptr;
    entities_.clear();
}
