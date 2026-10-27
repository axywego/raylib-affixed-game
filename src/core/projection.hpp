#pragma once

#include "raylib.h"

Vector3 ToWorld3D(Vector2 p, float height = 0.0f);

bool MouseToGround(const Camera3D& cam, Vector2& out);
