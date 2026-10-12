#ifndef ATTACK_CONE_H
#define ATTACK_CONE_H

#include <stdbool.h>

/*
 * Pure geometry for a cone-shaped melee attack.
 *
 * The cone starts at (origin_x, origin_y), points along
 * (direction_x, direction_y) and reaches out to range. A target
 * is a circle, so a large target is hit even when only its edge
 * lies inside the cone, and its radius also extends the reach.
 *
 * The direction does not need to be normalized. A zero or
 * invalid direction hits nothing.
 */
bool AttackCone_Contains(
    float origin_x,
    float origin_y,
    float direction_x,
    float direction_y,
    float target_x,
    float target_y,
    float target_radius,
    float range,
    float half_angle_degrees
);

#endif