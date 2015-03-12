#ifndef PICO_RAND_TYPES_H_
#define PICO_RAND_TYPES_H_

#include <stdint.h>

struct pico_rand_counter_fortuna {
    uint8_t values[16]; /* Fortuna requires a 128-bit counter */

};

void pico_rand_init_counter (struct pico_rand_counter_fortuna* counter);
void pico_rand_increment_counter (struct pico_rand_counter_fortuna* counter);
void pico_rand_reset_counter (struct pico_rand_counter_fortuna* counter);
int pico_rand_counter_is_zero (struct pico_rand_counter_fortuna* counter);

#endif
