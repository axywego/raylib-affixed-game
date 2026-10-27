#pragma once

#include <raylib.h>
#include <vector>

namespace game::world {
    class World;
}

namespace core::pathfinding {
    class Pathfinder {
    public:
        static std::vector<Vector3> findPath(const game::world::World& world, Vector3 start, Vector3 goal, float agentRadius = 0.4f);

        static bool isDirectPathClear(const game::world::World& world, Vector3 from, Vector3 to, float agentRadius = 0.4f);
    };
}
