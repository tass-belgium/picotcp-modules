#include "pico_rand_types.h"

inline void pico_rand_init_counter (struct pico_rand_counter_fortuna* counter) {
    int i;

    for (i = 0; i < 16; i++) {
        counter->values[i] = 0;

    }

}

inline void pico_rand_increment_counter (struct pico_rand_counter_fortuna* counter) {
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

int pico_rand_counter_is_zero (struct pico_rand_counter_fortuna* counter) {
    int i;

    for (i = 0; i < 16; i++) {
        if (counter->values[i] != 0) {
            return 0;

        }

    }

    return 1; /* None of the individual values are non-zero, so... */

}
