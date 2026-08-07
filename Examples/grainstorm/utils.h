#ifndef GRAINWAVES_UTILS
#define GRAINWAVES_UTILS

#include "daisysp.h"

using namespace daisysp;
using namespace std;

const float DEG_TO_TAU = PI_F * 2 / 360;

struct iVec2 {
    int x;
    int y;
};

inline float fwrap(float x, float min, float max) {
    if (max == min) return min;
    if (min > max) return fwrap(x, max, min);
    if (min < 0) return fwrap(x - min, min - min, max - min) + min;

    return (x >= 0 ? min : max) + fmodf(x, max - min);
}

inline int wrap(int x, int min, int max) {
    if (max == min) return min;
    if (min > max) return wrap(x, max, min);

    return (x >= 0 ? min : max) + x % (max - min);
}

inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

inline float randF(float min, float max) {
    return min + rand() * kRandFrac * (max - min);
}

inline float map_to_range(float fraction, float min, float max) {
    return min + fraction * (max - min);
}

inline float coerce_in_range(float value, float minimum, float maximum) {
    return max(min(value, maximum), minimum);
}

inline float modf(float x) {
    static float junk;
    return modf(x, &junk);
}

// Adds a deadzone centered around zero.
// Note: Reduces the range by the deadzone size
float with_dead_zone(float value, float deadzone_size) {
    float half_deadzone = deadzone_size * 0.5f;

    if (value > half_deadzone) {
        return value - half_deadzone;
    } else if (value < -half_deadzone) {
        return value + half_deadzone;
    } else {
        return 0;
    }
}


#endif