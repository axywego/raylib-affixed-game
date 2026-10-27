#pragma once

#include "raylib.h"

#include "../entity/entity.hpp"
#include <raymath.h>

namespace game {
    struct Frame {
        float rotationDeg{};
        float alpha{};
    };

    class Renderer {
    public:
        void draw(const entity::Entity& e, const Frame& f) {
            if(!e.visible) return;

            Vector3 pos = Vector3Lerp(e.prevTransform_.position, e.transform_.position, f.alpha);
            if (e.model_.meshCount > 0 && e.model_.meshes != nullptr) {
                DrawModelEx(e.model_, pos, {0, 1, 0}, f.rotationDeg, e.transform_.scale, WHITE);
            }
            e.draw(f.alpha);
        }
    };
}
