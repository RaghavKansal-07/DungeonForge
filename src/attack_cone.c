#include "attack_cone.h"

#include <math.h>

#define DEGREES_TO_RADIANS 0.0174532925f

bool AttackCone_Contains(
    float origin_x,
    float origin_y,
    float direction_x,
    float direction_y,
    float target_x,
    float target_y,
    float target_radius,
    float range,
    float half_angle_degrees)
{
    float direction_length =
        sqrtf(
            direction_x * direction_x +
            direction_y * direction_y);

    /*
     * Written so that NaN also fails this test.
     */
    if (!(direction_length > 0.0001f))
        return false;

    direction_x /= direction_length;
    direction_y /= direction_length;

    float to_x =
        target_x - origin_x;

    float to_y =
        target_y - origin_y;

    float distance =
        sqrtf(
            to_x * to_x +
            to_y * to_y);

    /*
     * Range check, including the target's radius.
     */
    if (distance > range + target_radius)
        return false;

    /*
     * A target on top of the origin is always inside.
     */
    if (distance <= 0.001f)
        return true;

    float dot =
        (direction_x * to_x +
         direction_y * to_y) /
        distance;

    if (dot > 1.0f)
        dot = 1.0f;

    if (dot < -1.0f)
        dot = -1.0f;

    float angle =
        acosf(dot);

    /*
     * A circle subtends an angle of asin(radius / distance),
     * so its edge can be inside the cone while its center
     * is slightly outside.
     */
    float ratio =
        target_radius / distance;

    if (ratio > 1.0f)
        ratio = 1.0f;

    if (ratio < 0.0f)
        ratio = 0.0f;

    float margin =
        asinf(ratio);

    return angle <=
           half_angle_degrees * DEGREES_TO_RADIANS +
               margin;
}