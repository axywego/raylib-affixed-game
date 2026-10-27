#pragma once

#include <raylib.h>

namespace game::collision {
    class AABB {
    public:
        AABB(Vector3 pos, Vector3 size);

        bool hasIntersectionWith(const AABB& other) const;
        void update(Vector3 pos, Vector3 size);
        void draw(float alpha) const;

        const Vector3& getMax() const { return secondPoint_; }
        const Vector3& getMin() const { return firstPoint_; }

    private:
        Vector3 prevFirstPoint_, prevSecondPoint_;
        Vector3 firstPoint_, secondPoint_;
    };
}
