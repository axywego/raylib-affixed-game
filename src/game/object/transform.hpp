#pragma once

#include <raymath.h>

namespace object {
    struct Transform {
        Vector3 position{};
        Vector3 rotation{};
        Vector3 scale{ 1.f, 1.f, 1.f };
    };
}
