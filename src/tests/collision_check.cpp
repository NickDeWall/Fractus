#include "render/collision_field.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    const unsigned char grid[] = {
        255, 255, 0, 0,
        255, 255, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };
    const auto at = [&](float u, float v) { return CollisionField::sampleGrid(grid, 4, 4, u, v); };

    assert(at(0.125f, 0.125f) == 1.0f);
    assert(at(0.875f, 0.125f) == 0.0f);
    assert(at(0.125f, 0.875f) == 0.0f);
    assert(std::fabs(at(0.5f, 0.125f) - 0.5f) < 1e-6f);
    assert(std::fabs(at(0.125f, 0.5f) - 0.5f) < 1e-6f);
    assert(at(-5.0f, -5.0f) == 1.0f);
    assert(at(5.0f, 5.0f) == 0.0f);
    assert(CollisionField::sampleGrid(nullptr, 0, 0, 0.5f, 0.5f) == 0.0f);

    std::puts("collision_check passed");
    return 0;
}