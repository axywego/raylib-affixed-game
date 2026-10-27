#include "collision.hpp"

#include <raylib.h>
#include <raymath.h>
#include <format>
#include <iostream>

namespace game::collision {
    AABB::AABB(Vector3 pos, Vector3 size) {
        // firstPoint_.x = pos.x - size.x / 2.f;
        firstPoint_ = Vector3Subtract(pos, size / 2);
        secondPoint_ = Vector3Add(pos, size / 2);
    }

    bool AABB::hasIntersectionWith(const AABB& other) const {
        return firstPoint_.x <= other.secondPoint_.x && other.firstPoint_.x <= secondPoint_.x
            && firstPoint_.y <= other.secondPoint_.y && other.firstPoint_.y <= secondPoint_.y
            && firstPoint_.z <= other.secondPoint_.z && other.firstPoint_.z <= secondPoint_.z;
    }

    void AABB::update(Vector3 pos, Vector3 size) {
        prevFirstPoint_ = firstPoint_; prevSecondPoint_ = secondPoint_;
        firstPoint_ = Vector3Subtract(pos, size / 2);
        secondPoint_ = Vector3Add(pos, size / 2);
    }

    void AABB::draw(float alpha) const {
        Vector3 size = Vector3Subtract(secondPoint_, firstPoint_);

        // std::cout << std::format("{} {} {}", size.x, size.y, size.z);

        Vector3 center = {
            firstPoint_.x + size.x / 2.0f,
            firstPoint_.y + size.y / 2.0f,
            firstPoint_.z + size.z / 2.0f
        };

        DrawCubeWiresV(center, size, GREEN);
    }
}
