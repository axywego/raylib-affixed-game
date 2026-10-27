#include "pathfinder.hpp"

#include "game/collision/collision.hpp"
#include "game/world/world.hpp"

#include <raymath.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

namespace core::pathfinding {
    namespace {
        constexpr float kStraightCost = 10.0f;
        constexpr float kDiagonalCost = 14.1421356f;
        constexpr int kNearestCellSearchRadius = 8;

        struct OpenEntry {
            float f = 0.0f;
            int index = -1;

            bool operator<(const OpenEntry& other) const {
                return f > other.f;
            }
        };

        inline int cellIndex(int x, int y, int width) {
            return y * width + x;
        }

        inline bool inBounds(const game::world::World& world, int x, int y) {
            return x >= 0 && y >= 0 && x < world.getWidth() && y < world.getHeight();
        }

        bool cellPassable(const game::world::World& world, int x, int y, float radius) {
            if (!inBounds(world, x, y) || !world.isWalkable(x, y)) return false;

            const Vector3 size = {radius * 2.0f, 1.0f, radius * 2.0f};
            const game::collision::AABB box(world.cellToWorld(x, y), size);
            return !world.overlaps(box);
        }

        bool nearestPassableCell(const game::world::World& world, int& x, int& y, float radius) {
            if (cellPassable(world, x, y, radius)) return true;

            for (int ring = 1; ring <= kNearestCellSearchRadius; ++ring) {
                for (int dz = -ring; dz <= ring; ++dz) {
                    for (int dx = -ring; dx <= ring; ++dx) {
                        if (std::max(std::abs(dx), std::abs(dz)) != ring) continue;

                        const int nx = x + dx;
                        const int ny = y + dz;
                        if (cellPassable(world, nx, ny, radius)) {
                            x = nx;
                            y = ny;
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        float octileHeuristic(int x1, int y1, int x2, int y2) {
            const int dx = std::abs(x1 - x2);
            const int dy = std::abs(y1 - y2);
            const int diagonal = std::min(dx, dy);
            const int straight = std::max(dx, dy) - diagonal;
            return static_cast<float>(straight) * kStraightCost + static_cast<float>(diagonal) * kDiagonalCost;
        }

        bool segmentClear(const game::world::World& world, Vector3 from, Vector3 to, float radius) {
            const float length = Vector3Distance(from, to);
            const float sampleStep = std::max(world.getCellSize() * 0.2f, 0.05f);
            const int steps = std::max(1, static_cast<int>(std::ceil(length / sampleStep)));

            const Vector3 size = {radius * 2.0f, 1.0f, radius * 2.0f};
            game::collision::AABB box(from, size);

            for (int i = 0; i <= steps; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(steps);
                box.update(Vector3Lerp(from, to, t), size);
                if (world.overlaps(box)) return false;
            }
            return true;
        }

        std::vector<Vector3> smoothPath(const game::world::World& world, const std::vector<Vector3>& points, float radius) {
            std::vector<Vector3> result;
            if (points.size() < 2) return result;

            result.reserve(points.size());
            size_t anchor = 0;

            while (anchor + 1 < points.size()) {
                size_t best = anchor + 1;

                for (size_t i = points.size(); i-- > anchor + 1;) {
                    if (segmentClear(world, points[anchor], points[i], radius)) {
                        best = i;
                        break;
                    }
                }

                result.push_back(points[best]);
                anchor = best;
            }

            return result;
        }
    }

    bool Pathfinder::isDirectPathClear(const game::world::World& world, Vector3 from, Vector3 to, float agentRadius) {
        return segmentClear(world, from, to, agentRadius);
    }

    std::vector<Vector3> Pathfinder::findPath(const game::world::World& world, Vector3 start, Vector3 goal, float agentRadius) {
        std::vector<Vector3> result;

        const int width = world.getWidth();
        const int height = world.getHeight();
        if (width <= 0 || height <= 0) return result;
        if (Vector3DistanceSqr(start, goal) < 0.000001f) return result;

        const Vector2 startCell = world.worldToCell(start);
        const Vector2 goalCell = world.worldToCell(goal);

        int sx = static_cast<int>(startCell.x);
        int sy = static_cast<int>(startCell.y);
        int gx = static_cast<int>(goalCell.x);
        int gy = static_cast<int>(goalCell.y);

        if (!nearestPassableCell(world, sx, sy, agentRadius)) return result;
        if (!nearestPassableCell(world, gx, gy, agentRadius)) return result;

        if (sx == gx && sy == gy) {
            if (segmentClear(world, start, goal, agentRadius)) {
                result.push_back(goal);
            }
            return result;
        }

        const int cellCount = width * height;
        std::vector<float> gScore(static_cast<size_t>(cellCount), std::numeric_limits<float>::infinity());
        std::vector<int> parent(static_cast<size_t>(cellCount), -1);
        std::vector<uint8_t> closed(static_cast<size_t>(cellCount), 0);

        std::priority_queue<OpenEntry> open;
        const int startIndex = cellIndex(sx, sy, width);
        const int goalIndexTarget = cellIndex(gx, gy, width);

        gScore[static_cast<size_t>(startIndex)] = 0.0f;
        open.push({octileHeuristic(sx, sy, gx, gy), startIndex});

        constexpr int kDx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
        constexpr int kDy[8] = {0, 0, 1, -1, 1, -1, 1, -1};

        int goalIndex = -1;

        while (!open.empty()) {
            const OpenEntry entry = open.top();
            open.pop();

            const int current = entry.index;
            if (closed[static_cast<size_t>(current)]) continue;
            closed[static_cast<size_t>(current)] = 1;

            if (current == goalIndexTarget) {
                goalIndex = current;
                break;
            }

            const int cx = current % width;
            const int cy = current / width;

            for (int i = 0; i < 8; ++i) {
                const int nx = cx + kDx[i];
                const int ny = cy + kDy[i];

                if (!cellPassable(world, nx, ny, agentRadius)) continue;

                const bool diagonal = kDx[i] != 0 && kDy[i] != 0;
                if (diagonal) {
                    if (!cellPassable(world, cx + kDx[i], cy, agentRadius)) continue;
                    if (!cellPassable(world, cx, cy + kDy[i], agentRadius)) continue;
                }

                const int neighbor = cellIndex(nx, ny, width);
                if (closed[static_cast<size_t>(neighbor)]) continue;

                const float stepCost = diagonal ? kDiagonalCost : kStraightCost;
                const float tentativeG = gScore[static_cast<size_t>(current)] + stepCost;

                if (tentativeG >= gScore[static_cast<size_t>(neighbor)]) continue;

                gScore[static_cast<size_t>(neighbor)] = tentativeG;
                parent[static_cast<size_t>(neighbor)] = current;
                open.push({tentativeG + octileHeuristic(nx, ny, gx, gy), neighbor});
            }
        }

        if (goalIndex < 0) return result;

        std::vector<std::pair<int, int>> cells;
        for (int current = goalIndex; current >= 0; current = parent[static_cast<size_t>(current)]) {
            cells.push_back({current % width, current / width});
            if (current == startIndex) break;
        }
        std::reverse(cells.begin(), cells.end());

        std::vector<Vector3> points;
        points.reserve(cells.size() + 2);
        points.push_back(start);

        for (const auto& cell : cells) {
            points.push_back(world.cellToWorld(cell.first, cell.second));
        }

        const Vector3 goalCenter = world.cellToWorld(gx, gy);
        if (segmentClear(world, goalCenter, goal, agentRadius)) {
            if (Vector3DistanceSqr(goalCenter, goal) > 0.000001f) {
                points.push_back(goal);
            }
        }

        return smoothPath(world, points, agentRadius);
    }
}
