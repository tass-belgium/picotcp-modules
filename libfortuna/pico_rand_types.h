#ifndef PICO_RAND_TYPES_H_
#define PICO_RAND_TYPES_H_

#include <stdint.h>

struct counter_fortuna {
    uint8_t values[16]; /* Fortuna requires a 128-bit counter */

};

void increment_counter (struct counter_fortuna* counter);
void reset_counter (struct counter_fortuna* counter);
int counter_is_zero (struct counter_fortuna* counter);

#endif
