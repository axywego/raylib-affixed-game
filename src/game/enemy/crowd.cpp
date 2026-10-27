#include "crowd.hpp"

#include "enemy.hpp"
#include "game/collision/collision.hpp"
#include "game/world/world.hpp"

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace game::entity::enemy {
    namespace {
        constexpr float kNeighborRadius = 1.2f;
        constexpr float kMaxNeighborDistance = 10.0f;
        constexpr float kOverlapDistance = Enemy::kSize + 0.20f;
        constexpr int kOverlapIterations = 32;
        constexpr float kCorrectionDamping = 1.0f;
        constexpr int kFinalPushPasses = 12;
        constexpr float kFinalPushDamping = 0.5f;
        constexpr int kFinalEnemyIterations = 6;
        constexpr float kCarryPenalty = 2.0f;
        constexpr float kPushAnglePenalty = 0.02f;
        constexpr float kGeneralPushAngleLimit = 45.0f;
        constexpr float kCarryEscapeFrontDot = 0.95f;
        constexpr float kCarryEscapeNeighborRadius = 1.5f;
        constexpr float kCarryEscapeSideBonus = 0.5f;
        constexpr float kSideAngleDegrees = 90.0f;

        template <typename Fn>
        void forEachNeighbor(const game::world::World& world,
                             const std::vector<Enemy*>& enemies,
                             const std::vector<std::vector<int>>& buckets,
                             size_t self,
                             Vector3 position,
                             float radius,
                             Fn&& fn) {
            const Vector2 cell = world.worldToCell(position);
            const int cx = static_cast<int>(cell.x);
            const int cy = static_cast<int>(cell.y);
            const float cellSize = world.getCellSize();
            const int range = std::max(1, static_cast<int>(std::ceil(radius / cellSize)));
            const float radiusSqr = radius * radius;

            for (int dz = -range; dz <= range; ++dz) {
                for (int dx = -range; dx <= range; ++dx) {
                    const int x = cx + dx;
                    const int y = cy + dz;
                    if (x < 0 || y < 0 || x >= world.getWidth() || y >= world.getHeight()) continue;

                    const auto& bucket = buckets[static_cast<size_t>(y * world.getWidth() + x)];
                    for (int index : bucket) {
                        if (static_cast<size_t>(index) == self) continue;

                        Vector3 diff = Vector3Subtract(position, enemies[static_cast<size_t>(index)]->transform_.position);
                        diff.y = 0.0f;
                        if (Vector3LengthSqr(diff) > radiusSqr) continue;

                        fn(static_cast<size_t>(index), diff);
                    }
                }
            }
        }
    }

    void EnemyCrowd::add(Enemy* enemy) {
        if (!enemy) return;
        enemies_.push_back(enemy);
    }

    void EnemyCrowd::clear() {
        enemies_.clear();
        separation_.clear();
        buckets_.clear();
    }

    void EnemyCrowd::update(float dt, const game::world::World& world, Vector3 playerPosition, float playerRadius, Vector3 playerVelocity) {
        if (enemies_.empty()) return;

        rebuildGrid(world);
        computeSeparation(world);

        for (size_t i = 0; i < enemies_.size(); ++i) {
            enemies_[i]->update(dt, world, separation_[i]);
        }

        resolveOverlaps(world, playerPosition, playerRadius, playerVelocity);
    }

    void EnemyCrowd::rebuildGrid(const game::world::World& world) {
        const int width = world.getWidth();
        const int height = world.getHeight();
        if (width <= 0 || height <= 0) return;

        const size_t cellCount = static_cast<size_t>(width) * static_cast<size_t>(height);
        if (buckets_.size() != cellCount) buckets_.resize(cellCount);

        for (auto& bucket : buckets_) bucket.clear();

        for (size_t i = 0; i < enemies_.size(); ++i) {
            const Vector2 cell = world.worldToCell(enemies_[i]->transform_.position);
            const int x = std::clamp(static_cast<int>(cell.x), 0, width - 1);
            const int y = std::clamp(static_cast<int>(cell.y), 0, height - 1);
            buckets_[static_cast<size_t>(y * width + x)].push_back(static_cast<int>(i));
        }
    }

    void EnemyCrowd::computeSeparation(const game::world::World& world) {
        separation_.assign(enemies_.size(), Vector3{0.0f, 0.0f, 0.0f});

        for (size_t i = 0; i < enemies_.size(); ++i) {
            Vector3 force{};

            forEachNeighbor(world, enemies_, buckets_, i, enemies_[i]->transform_.position, kNeighborRadius,
                [&](size_t, Vector3 diff) {
                    const float distance = Vector3Length(diff);
                    if (distance < 0.0001f) return;

                    const float strength = 1.0f - distance / kNeighborRadius;
                    force = Vector3Add(force, Vector3Scale(diff, strength / distance));
                });

            const float length = Vector3Length(force);
            if (length > 1.0f) {
                force = Vector3Scale(force, 1.0f / length);
            }

            separation_[i] = force;
        }
    }

    void EnemyCrowd::pushOutOfPlayer(const game::world::World& world, Vector3 playerPosition, float playerRadius, float damping, Vector3 playerVelocity, bool allowCarryEscape) {
        const float playerOverlapDistance = Enemy::kHalfSize + playerRadius;
        const float cellSize = world.getCellSize();
        const int range = std::max(1, static_cast<int>(std::ceil(kNeighborRadius / cellSize)));
        const int width = world.getWidth();
        const int height = world.getHeight();

        rebuildGrid(world);

        auto minDistanceToOther = [&](Vector3 position, size_t selfIndex) {
            const Vector2 cell = world.worldToCell(position);
            const int cx = static_cast<int>(cell.x);
            const int cy = static_cast<int>(cell.y);
            float minDistance = kMaxNeighborDistance;

            for (int dz = -range; dz <= range; ++dz) {
                for (int dx = -range; dx <= range; ++dx) {
                    const int x = cx + dx;
                    const int y = cy + dz;
                    if (x < 0 || y < 0 || x >= width || y >= height) continue;

                    const auto& bucket = buckets_[static_cast<size_t>(y * width + x)];
                    for (int rawIndex : bucket) {
                        const size_t j = static_cast<size_t>(rawIndex);
                        if (j == selfIndex) continue;

                        Vector3 diff = Vector3Subtract(position, enemies_[j]->transform_.position);
                        diff.y = 0.0f;
                        minDistance = std::min(minDistance, Vector3Length(diff));
                    }
                }
            }

            return minDistance;
        };

        constexpr float kPlayerPushAngles[] = {
            0.0f, 22.5f, -22.5f, 45.0f, -45.0f, 90.0f, -90.0f
        };

        Vector3 playerForward = {0.0f, 0.0f, 0.0f};
        const bool playerMoving = Vector3LengthSqr(playerVelocity) > 0.0001f;
        if (playerMoving) {
            playerForward = Vector3Normalize(playerVelocity);
            playerForward.y = 0.0f;
        }

        for (size_t i = 0; i < enemies_.size(); ++i) {
            Enemy* enemy = enemies_[i];
            Vector3 diff = Vector3Subtract(enemy->transform_.position, playerPosition);
            diff.y = 0.0f;
            const float distance = Vector3Length(diff);
            if (distance >= playerOverlapDistance) continue;

            Vector3 radial = {1.0f, 0.0f, 0.0f};
            if (i < pushReference_.size()) {
                Vector3 referenceDiff = Vector3Subtract(pushReference_[i], playerPosition);
                referenceDiff.y = 0.0f;
                const float referenceDistance = Vector3Length(referenceDiff);
                if (referenceDistance > 0.0001f) {
                    radial = Vector3Scale(referenceDiff, 1.0f / referenceDistance);
                } else if (distance > 0.0001f) {
                    radial = Vector3Scale(diff, 1.0f / distance);
                }
            } else if (distance > 0.0001f) {
                radial = Vector3Scale(diff, 1.0f / distance);
            }

            Vector3 bestTarget = Vector3Add(playerPosition, Vector3Scale(radial, playerOverlapDistance));
            float bestScore = -1e9f;
            const float frontness = playerMoving ? std::max(0.0f, Vector3DotProduct(radial, playerForward)) : 0.0f;
            bool isolated = true;
            if (allowCarryEscape && frontness > kCarryEscapeFrontDot) {
                for (size_t j = 0; j < enemies_.size(); ++j) {
                    if (j == i) continue;
                    Vector3 otherDiff = Vector3Subtract(enemies_[j]->transform_.position, enemy->transform_.position);
                    otherDiff.y = 0.0f;
                    if (Vector3LengthSqr(otherDiff) < kCarryEscapeNeighborRadius * kCarryEscapeNeighborRadius) {
                        isolated = false;
                        break;
                    }
                }
            }
            const bool directFront = allowCarryEscape && playerMoving && frontness > kCarryEscapeFrontDot && isolated;

            for (float angleDeg : kPlayerPushAngles) {
                if (std::fabs(angleDeg) > kGeneralPushAngleLimit && !(directFront && std::fabs(angleDeg) >= kSideAngleDegrees - 0.1f)) continue;
                const float angle = angleDeg * DEG2RAD;
                const float c = std::cos(angle);
                const float s = std::sin(angle);
                const Vector3 direction = {
                    radial.x * c - radial.z * s,
                    0.0f,
                    radial.x * s + radial.z * c
                };

                const Vector3 target = Vector3Add(playerPosition, Vector3Scale(direction, playerOverlapDistance));
                collision::AABB targetBox(target, {Enemy::kSize, 1.0f, Enemy::kSize});
                if (world.overlaps(targetBox)) continue;

                float score = minDistanceToOther(target, i) - std::fabs(angleDeg) * kPushAnglePenalty;
                if (directFront && std::fabs(angleDeg) >= kSideAngleDegrees - 0.1f) {
                    score += kCarryEscapeSideBonus;
                }

                if (playerMoving && frontness > 0.0f) {
                    const float forwardDot = std::max(0.0f, Vector3DotProduct(direction, playerForward));
                    score -= forwardDot * kCarryPenalty * frontness;
                }

                if (score > bestScore) {
                    bestScore = score;
                    bestTarget = target;
                }
            }

            enemy->move(Vector3Scale(Vector3Subtract(bestTarget, enemy->transform_.position), damping), world);
            rebuildGrid(world);
        }
    }

    bool EnemyCrowd::resolveEnemyPass(const game::world::World& world, Vector3 playerPosition, float playerRadius) {
        const float playerClearance = Enemy::kHalfSize + playerRadius;
        const float cellSize = world.getCellSize();
        const int range = std::max(1, static_cast<int>(std::ceil(kNeighborRadius / cellSize)));
        const int width = world.getWidth();
        const int height = world.getHeight();

        rebuildGrid(world);
        bool anyOverlap = false;

        auto moveEnemy = [&](Enemy* enemy, Vector3 delta) {
            Vector3 target = Vector3Add(enemy->transform_.position, delta);
            Vector3 diff = Vector3Subtract(target, playerPosition);
            diff.y = 0.0f;
            const float distance = Vector3Length(diff);
            if (distance < playerClearance) {
                Vector3 radial = {1.0f, 0.0f, 0.0f};
                if (distance > 0.0001f) {
                    radial = Vector3Scale(diff, 1.0f / distance);
                }
                target = Vector3Add(playerPosition, Vector3Scale(radial, playerClearance));
                delta = Vector3Subtract(target, enemy->transform_.position);
            }
            enemy->move(delta, world);
        };

        for (size_t i = 0; i < enemies_.size(); ++i) {
            Enemy* self = enemies_[i];
            const Vector2 selfCell = world.worldToCell(self->transform_.position);
            const int cx = static_cast<int>(selfCell.x);
            const int cy = static_cast<int>(selfCell.y);

            for (int dz = -range; dz <= range; ++dz) {
                for (int dx = -range; dx <= range; ++dx) {
                    const int x = cx + dx;
                    const int y = cy + dz;
                    if (x < 0 || y < 0 || x >= width || y >= height) continue;

                    const auto& bucket = buckets_[static_cast<size_t>(y * width + x)];
                    for (int rawIndex : bucket) {
                        const size_t j = static_cast<size_t>(rawIndex);
                        if (j <= i) continue;

                        Enemy* other = enemies_[j];
                        Vector3 diff = Vector3Subtract(self->transform_.position, other->transform_.position);
                        diff.y = 0.0f;
                        if (Vector3LengthSqr(diff) > kNeighborRadius * kNeighborRadius) continue;

                        const float distance = Vector3Length(diff);
                        if (distance >= kOverlapDistance) continue;

                        anyOverlap = true;

                        Vector3 direction = {1.0f, 0.0f, 0.0f};
                        if (distance > 0.0001f) {
                            direction = Vector3Scale(diff, 1.0f / distance);
                        } else if ((i + j) % 2 != 0) {
                            direction.x = -1.0f;
                        }

                        const Vector3 push = Vector3Scale(direction, (kOverlapDistance - distance) * 0.5f * kCorrectionDamping);
                        moveEnemy(self, push);
                        moveEnemy(other, Vector3Scale(push, -1.0f));
                    }
                }
            }
        }

        return anyOverlap;
    }

    void EnemyCrowd::resolveOverlaps(const game::world::World& world, Vector3 playerPosition, float playerRadius, Vector3 playerVelocity) {
        pushReference_.resize(enemies_.size());
        for (size_t i = 0; i < enemies_.size(); ++i) {
            pushReference_[i] = enemies_[i]->transform_.position;
        }

        pushOutOfPlayer(world, playerPosition, playerRadius, 1.0f, playerVelocity, true);
        for (size_t i = 0; i < enemies_.size(); ++i) {
            pushReference_[i] = enemies_[i]->transform_.position;
        }

        for (int iteration = 0; iteration < kOverlapIterations; ++iteration) {
            if (!resolveEnemyPass(world, playerPosition, playerRadius)) break;
        }

        for (int pass = 0; pass < kFinalPushPasses; ++pass) {
            pushOutOfPlayer(world, playerPosition, playerRadius, kFinalPushDamping, playerVelocity);

            for (int iteration = 0; iteration < kFinalEnemyIterations; ++iteration) {
                if (!resolveEnemyPass(world, playerPosition, playerRadius)) break;
            }
        }

        for (auto* enemy : enemies_) enemy->syncCollision();
    }
}
