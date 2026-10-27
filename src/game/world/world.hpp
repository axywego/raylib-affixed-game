#pragma once

#include <raylib.h>
#include "../collision/collision.hpp"
#include <vector>

namespace game::world {
    enum class TileType {
        Empty, Wall, Floor
    };

    struct Cell {
        TileType type = TileType::Empty;
        bool walkable {true};
    };

    // struct WorldObj {
    //     WorldObj(Vector3 pos, Vector3 size);
    //     void draw() const;

    //     object::Transform transform;
    //     collision::AABB collision;
    //     Model* model {nullptr};
    // };

    class World {
    public:
        void init(int width, int height, float cellSize, Vector3 origin = {0.f, 0.f, 0.f});
        void setTile(int x, int y, TileType type);
        void setTileWorld(Vector3 pos, TileType type);

        bool isWalkable(int x, int y) const;
        bool isWalkableWorld(Vector3 pos) const;

        Vector2 worldToCell(Vector3 pos) const;
        Vector3 cellToWorld(int x, int y) const;

        bool blockedBetween(Vector3 from, Vector3 to) const;

        void draw() const;

        int getWidth() const { return width_; }
        int getHeight() const { return height_; }
        float getCellSize() const { return cellSize_; }

        bool overlaps(const collision::AABB& box) const;
        // bool overlaps(Vector3 prevPos, Vector3 currPos) const;
        // std::vector<const WorldObj*> overlapsAll(const collision::AABB& box) const;
    private:
        int width_ {0}, height_ {0};
        float cellSize_ {1.f};
        Vector3 origin_ {0.f, 0.f, 0.f};
        std::vector<Cell> grid_; // плоский массив y * width + x
    };
}
