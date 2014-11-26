#include "pico_rand_types.h"

inline void init_counter (struct counter_fortuna* counter) {
    int i;

    for (i = 0; i < 16; i++) {
        counter->values[i] = 0;

    }

}

inline void increment_counter (struct counter_fortuna* counter) {
    int i;

    for (i = 0; i < 16; i++) {
        if (counter->values[i] < 0xFF) { /* Not at maximum value */
            counter->values[i]++;
            return; /* Only return once we've found one not ready to overflow */

        } else {
            counter->values[i] = 0;

        }

    }

}

int counter_is_zero (struct counter_fortuna* counter) {
    int i;

    for (i = 0; i < 16; i++) {
        if (counter->values[i] != 0) {
            return 0;

        }

    }

    return 1; /* None of the individual values are non-zero, so... */

}
