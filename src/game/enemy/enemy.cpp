#include "enemy.hpp"

#include "core/pathfinding/pathfinder.hpp"
#include "game/player/player.hpp"

#include <raymath.h>

#include <cmath>

namespace game::entity::enemy {
    namespace {
        constexpr float kMoveSpeed = 3.0f; // 6
        constexpr float kTurnSpeed = 10.0f; // 14
        constexpr float kRepathInterval = 0.3f;
        constexpr float kRepathGoalDistance = 1.0f;
        constexpr float kArriveRadius = 0.2f;
        constexpr float kStopDistance = 1.2f;
        constexpr float kStuckTime = 0.35f;
        constexpr float kSeparationWeight = 0.9f;
        constexpr float kEpsilon = 0.000001f;

        Vector3 flatDirection(Vector3 from, Vector3 to) {
            Vector3 dir = Vector3Subtract(to, from);
            dir.y = 0.0f;

            const float length = Vector3Length(dir);
            if (length <= kEpsilon) return {0.0f, 0.0f, 0.0f};

            return Vector3Scale(dir, 1.0f / length);
        }

        Vector3 turnTowards(Vector3 current, Vector3 desired, float maxTurn) {
            if (Vector3LengthSqr(desired) <= kEpsilon) return current;
            if (Vector3LengthSqr(current) <= kEpsilon) return desired;

            const float currentAngle = std::atan2(current.z, current.x);
            const float desiredAngle = std::atan2(desired.z, desired.x);
            const float delta = std::atan2(std::sin(desiredAngle - currentAngle), std::cos(desiredAngle - currentAngle));
            const float angle = currentAngle + Clamp(delta, -maxTurn, maxTurn);

            return {std::cos(angle), 0.0f, std::sin(angle)};
        }
    }

    Enemy::Enemy(Vector3 pos, const game::Player* player): player_(player) {
        transform_.position = pos;
        collision_ = collision::AABB(pos, {kSize, 1.0f, kSize});
    }

    void Enemy::update(float dt, const world::World& world) {
        update(dt, world, {0.0f, 0.0f, 0.0f});
    }

    void Enemy::update(float dt, const world::World& world, Vector3 separation) {
        prevTransform_ = transform_;
        if (!player_) return;

        const Vector3 goal = player_->transform_.position;
        const float distanceToGoal = Vector3Distance(transform_.position, goal);
        const bool direct = core::pathfinding::Pathfinder::isDirectPathClear(world, transform_.position, goal, kHalfSize);

        repathTimer_ -= dt;

        if (direct) {
            path_.clear();
            pathIndex_ = 0;
            repathTimer_ = 0.0f;
        } else if (repathTimer_ <= 0.0f
                   || Vector3DistanceSqr(goal, pathGoal_) > kRepathGoalDistance * kRepathGoalDistance) {
            path_ = core::pathfinding::Pathfinder::findPath(world, transform_.position, goal, kHalfSize);
            pathIndex_ = 0;
            pathGoal_ = goal;
            repathTimer_ = kRepathInterval + static_cast<float>(GetRandomValue(0, 5)) / 100.0f;
        }

        Vector3 target = goal;
        bool moving = true;

        if (!path_.empty()) {
            while (pathIndex_ < path_.size()) {
                const Vector3 toWaypoint = Vector3Subtract(path_[pathIndex_], transform_.position);
                const Vector3 flat = {toWaypoint.x, 0.0f, toWaypoint.z};
                if (Vector3LengthSqr(flat) > kArriveRadius * kArriveRadius) break;
                ++pathIndex_;
            }

            while (pathIndex_ + 1 < path_.size()
                   && core::pathfinding::Pathfinder::isDirectPathClear(world, transform_.position, path_[pathIndex_ + 1], kHalfSize)) {
                ++pathIndex_;
            }

            if (pathIndex_ < path_.size()) {
                target = path_[pathIndex_];
            } else {
                path_.clear();
                pathIndex_ = 0;
                repathTimer_ = 0.0f;
            }
        }

        if (direct && distanceToGoal <= kStopDistance) {
            moving = false;
        }

        Vector3 desired = flatDirection(transform_.position, target);

        if (Vector3LengthSqr(desired) > kEpsilon && Vector3LengthSqr(separation) > kEpsilon) {
            const float along = Vector3DotProduct(separation, desired);
            separation = Vector3Subtract(separation, Vector3Scale(desired, along));
            desired = Vector3Add(desired, Vector3Scale(separation, kSeparationWeight));
            desired = Vector3Normalize(desired);
        }

        if (Vector3LengthSqr(desired) > kEpsilon) {
            curDir_ = turnTowards(curDir_, desired, kTurnSpeed * dt);
        }

        const float alignment = moving ? Clamp(Vector3DotProduct(curDir_, desired), 0.0f, 1.0f) : 0.0f;
        const float step = kMoveSpeed * alignment * dt;
        const Vector3 startPosition = transform_.position;

        if (step > kEpsilon) {
            move(Vector3Scale(curDir_, step), world);
        }

        const float actualStep = Vector3Distance(transform_.position, startPosition);
        if (step > kEpsilon && alignment > 0.5f && actualStep < step * 0.25f) {
            stuckTimer_ += dt;
            if (stuckTimer_ >= kStuckTime) {
                stuckTimer_ = 0.0f;
                repathTimer_ = 0.0f;
                path_.clear();
                pathIndex_ = 0;
            }
        } else {
            stuckTimer_ = 0.0f;
        }

        syncCollision();

        if (Vector3LengthSqr(curDir_) > kEpsilon) {
            facing_ = {curDir_.x, curDir_.z};
        }
    }

    void Enemy::move(Vector3 delta, const world::World& world) {
        if (std::fabs(delta.x) > kEpsilon) {
            Vector3 next = transform_.position;
            next.x += delta.x;
            collision_.update(next, {kSize, 1.0f, kSize});
            if (!world.overlaps(collision_)) {
                transform_.position.x = next.x;
            }
        }

        if (std::fabs(delta.z) > kEpsilon) {
            Vector3 next = transform_.position;
            next.z += delta.z;
            collision_.update(next, {kSize, 1.0f, kSize});
            if (!world.overlaps(collision_)) {
                transform_.position.z = next.z;
            }
        }

        syncCollision();
    }

    void Enemy::syncCollision() {
        collision_.update(transform_.position, {kSize, 1.0f, kSize});
    }

    void Enemy::draw(float alpha) const {
        DrawCube(renderPosition(alpha), kSize, 1.0f, kSize, YELLOW);
    }
}
