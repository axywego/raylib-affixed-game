#pragma once

#include "game/world/world.hpp"
#include "raylib.h"
#include <raymath.h>
#include "../object/transform.hpp"

#include "game/collision/collision.hpp"

#include <cmath>

namespace game::entity {
    class Entity {
    public:
        virtual ~Entity() = default;
        virtual void update(float dt, const world::World& world) = 0;
        virtual void animate(float dt) {}
        virtual bool alive() const { return true; }

        virtual void draw(float alpha) const { collision_.draw(alpha); }

        Vector3 renderPosition(float alpha) const {
            return Vector3Lerp(prevTransform_.position, transform_.position, alpha);
        }

        // поворот модели по Y из направления взгляда (XZ) + поправка модели
        float rotationDeg() const {
            return std::atan2(-facing_.y, facing_.x) * RAD2DEG + yawOffset;
        }

        float yawOffset {0.f}; //угол поворота по Y (поправка модели)
        Vector2 facing_ {1.f, 0.f}; //направление взгляда в плоскости XZ

        object::Transform transform_, prevTransform_;
        Model model_ {};
        bool visible {true};
        collision::AABB collision_{{},{}};

        const Vector3 colliderSize_ {1.5f, 2.0f, 1.5f};
    };
}
