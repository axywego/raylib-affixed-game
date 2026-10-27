#include "projection.hpp"

Vector3 ToWorld3D(Vector2 p, float height) {
    return {p.x, height, p.y};
}

bool MouseToGround(const Camera3D& cam, Vector2& out) {
    const Ray ray = GetMouseRay(GetMousePosition(), cam);
    if (ray.direction.y >= -0.0001f) return false;
    const float t = -ray.position.y / ray.direction.y;
    out = {ray.position.x + ray.direction.x * t, ray.position.z + ray.direction.z * t};
    return true;
}
