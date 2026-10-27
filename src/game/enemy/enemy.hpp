#pragma once

#include "game/entity/entity.hpp"
#include "game/world/world.hpp"
#include <vector>

namespace game {
    class Player;
}

namespace game::entity::enemy {
    class Enemy : public Entity {
    public:
        static constexpr float kSize = 0.8f;
        static constexpr float kHalfSize = kSize * 0.5f;

        Enemy(Vector3 pos, const game::Player* player);
        void update(float dt, const world::World& world) override;
        void update(float dt, const world::World& world, Vector3 separation);
        void draw(float alpha) const override;

        void move(Vector3 delta, const world::World& world);
        void syncCollision();

    private:
        float health_ {100.f};
        const game::Player* player_ {nullptr};

        std::vector<Vector3> path_;
        size_t pathIndex_ {0};
        Vector3 pathGoal_ {};
        float repathTimer_ {0.0f};
        float stuckTimer_ {0.0f};
        Vector3 curDir_ {0.0f, 0.0f, 0.0f};
    };
}
