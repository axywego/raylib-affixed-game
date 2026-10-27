#include "world.hpp"

#include <bits/types/wint_t.h>
#include <raylib.h>
#include <raymath.h>

namespace game::world {
    void World::init(int width, int height, float cellSize, Vector3 origin) {
        width_ = width;
        height_ = height;
        cellSize_ = cellSize;
        origin_ = origin;

        grid_.resize(width * height);
    }

    void World::setTile(int x, int y, TileType type) {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
        auto& cell = grid_[y * width_ + x];
        cell.type = type;
        cell.walkable = (type != TileType::Wall);
    }

    void World::setTileWorld(Vector3 pos, TileType type) {
        const auto c = worldToCell(pos);
        int x = c.x;
        int y = c.y;

        if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
        auto& cell = grid_[y * width_ + x];
        cell.type = type;
        cell.walkable = (type != TileType::Wall);
    }

    bool World::isWalkable(int x, int y) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return false;
        return grid_[y * width_ + x].walkable;
    }

    bool World::isWalkableWorld(Vector3 pos) const {
        const auto cell = worldToCell(pos);
        int x = cell.x;
        int y = cell.y;
        if (x < 0 || x >= width_ || y < 0 || y >= height_) return false;
        return grid_[y * width_ + x].walkable;
    }

    Vector2 World::worldToCell(Vector3 pos) const {
        pos = Vector3Subtract(pos, origin_);
        float x = std::floor(pos.x / cellSize_);
        float z = std::floor(pos.z / cellSize_);
        return {x, z};
    }

    Vector3 World::cellToWorld(int x, int y) const {
        return {origin_.x + x * cellSize_ + cellSize_ * 0.5f, origin_.y, origin_.z + y * cellSize_ + cellSize_ * 0.5f};
    }

    bool World::blockedBetween(Vector3 from, Vector3 to) const {
        const float length = Vector3Length(Vector3Subtract(to, from));
        if(length < 0.0001f) return !isWalkableWorld(from);

        const float step = cellSize_ * 0.25f;
        int steps = (int)(length / step) + 1;
        if (steps < 2) steps = 2;
        if (steps > 256) steps = 256;

        for (int i = 1; i <= steps; ++i) {
            const float t = static_cast<float>(i) / steps;
            const Vector3 p = Vector3Lerp(from, to, t);
            if (!isWalkableWorld(p)) return true;
        }

        return false;
    }

    void World::draw() const {
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                const auto& cell = grid_[y * width_ + x];
                const auto pos = cellToWorld(x, y);
                switch (cell.type) {
                    case game::world::TileType::Empty:
                        break;
                    case game::world::TileType::Floor:
                        // DrawCube(pos, cellSize_, 0.1f, cellSize_, WHITE);
                        break;
                    case game::world::TileType::Wall:
                        DrawCube(pos, cellSize_, cellSize_, cellSize_, BLUE);
                        break;
                }
            }
        }
    }

    bool World::overlaps(const collision::AABB& box) const {
        const Vector3 min = box.getMin();
        const Vector3 max = box.getMax();

        const auto c1 = worldToCell({min.x, 0.0f, min.z});
        const auto c2 = worldToCell({max.x, 0.0f, max.z});

        int cx0 = (int)std::min(c1.x, c2.x);
        int cx1 = (int)std::max(c1.x, c2.x);
        int cz0 = (int)std::min(c1.y, c2.y);
        int cz1 = (int)std::max(c1.y, c2.y);

        for (int cz = cz0; cz <= cz1; ++cz) {
            for (int cx = cx0; cx <= cx1; ++cx) {
                if (cx < 0 || cx >= width_ || cz < 0 || cz >= height_) return true;
                if (!grid_[cz * width_ + cx].walkable) return true;
            }
        }
        return false;
    }

    // bool World::overlaps(Vector3 prevPos, Vector3 currPos) const {
    //     const float dx = currPos.x - prevPos.x;
    //     const float dy = currPos.y - prevPos.y;
    //     const float dz = currPos.z - prevPos.z;

    //     for (const auto& object : objects_) {
    //         const auto& minPoint = object.collision.firstPoint_;
    //         const auto& maxPoint = object.collision.secondPoint_;

    //         if (std::max(prevPos.x, currPos.x) < minPoint.x || std::min(prevPos.x, currPos.x) > maxPoint.x ||
    //             std::max(prevPos.y, currPos.y) < minPoint.y || std::min(prevPos.y, currPos.y) > maxPoint.y ||
    //             std::max(prevPos.z, currPos.z) < minPoint.z || std::min(prevPos.z, currPos.z) > maxPoint.z) {
    //             continue;
    //         }

    //         float t_min = 0.f;
    //         float t_max = 1.f;
    //         bool intersect_this_object = true;

    //         const std::array<std::tuple<float, float, float, float>, 3> directions{{
    //             {dx, prevPos.x, minPoint.x, maxPoint.x},
    //             {dy, prevPos.y, minPoint.y, maxPoint.y},
    //             {dz, prevPos.z, minPoint.z, maxPoint.z}
    //         }};

    //         for (const auto& [d, p, min, max] : directions) {
    //             if (std::abs(d) < 1e-7f) {
    //                 if (p < min || p > max) {
    //                     intersect_this_object = false;
    //                     break;
    //                 }
    //             }
    //             else {
    //                 const float inv_d = 1.f / d;
    //                 float t1 = (min - p) * inv_d;
    //                 float t2 = (max - p) * inv_d;

    //                 if (t1 > t2) {
    //                     std::swap(t1, t2);
    //                 }

    //                 t_min = std::max(t_min, t1);
    //                 t_max = std::min(t_max, t2);

    //                 if (t_min > t_max) {
    //                     intersect_this_object = false;
    //                     break;
    //                 }
    //             }
    //         }

    //         if (intersect_this_object) {
    //             return true;
    //         }
    //     }

    //     return false;
    // }


    // std::vector<const WorldObj*> World::overlapsAll(const collision::AABB& box) const {
    //     std::vector<const WorldObj*> overlapsedObjects;

    //     for(const auto& object : objects_) {
    //         if(box.hasIntersectionWith(object.collision)) overlapsedObjects.push_back(&object);
    //     }
    //     return overlapsedObjects;
    // }
}
