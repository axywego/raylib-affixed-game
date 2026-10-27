#pragma once

#include <raylib.h>
#include <vector>

namespace game::world {
    class World;
}

namespace game::entity::enemy {
    class Enemy;

    class EnemyCrowd {
    public:
        void add(Enemy* enemy);
        void clear();
        void update(float dt, const game::world::World& world, Vector3 playerPosition, float playerRadius, Vector3 playerVelocity = {0.0f, 0.0f, 0.0f});

        const std::vector<Enemy*>& enemies() const { return enemies_; }

    private:
        void rebuildGrid(const game::world::World& world);
        void computeSeparation(const game::world::World& world);
        void pushOutOfPlayer(const game::world::World& world, Vector3 playerPosition, float playerRadius, float damping = 1.0f, Vector3 playerVelocity = {0.0f, 0.0f, 0.0f}, bool allowCarryEscape = false);
        bool resolveEnemyPass(const game::world::World& world, Vector3 playerPosition, float playerRadius);
        void resolveOverlaps(const game::world::World& world, Vector3 playerPosition, float playerRadius, Vector3 playerVelocity);

        std::vector<Enemy*> enemies_;
        std::vector<Vector3> separation_;
        std::vector<Vector3> pushReference_;
        std::vector<std::vector<int>> buckets_;
    };
}
