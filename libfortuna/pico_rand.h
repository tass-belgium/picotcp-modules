/* An implementation of the Fortuna PRNG for PicoTCP */

#ifndef PICO_RAND_H_
#define PICO_RAND_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PICO_ZALLOC(x) calloc(x, 1) // TODO

#include "cyassl/ctaocrypt/aes.h"
#include "cyassl/ctaocrypt/sha.h"

#include "pico_rand_types.h"

#define PICO_RAND_POOL_COUNT            16
#define PICO_RAND_MINIMUM_RESEED_MS     100
#define PICO_RAND_MAX_REQUEST_BYTES     1048576
 
/* Define that we have access to load/store for the seed in this environment. */
// #define PICO_RAND_SEED_PERSISTENT

/* Internal generator state */
struct pico_rand_generator_state {
    /* Fortuna internals */
	uint8_t* key; /* 32 byte key */
	struct counter_fortuna* counter;
	uint8_t* pool;
    int last_reseed_time;

    /* AES and SHA internal stuff */
    Aes* aes;
    Sha* sha;

};

/* For setting up the generator and feeding entropy */
int pico_rand_init(struct pico_rand_generator_state* state);
void pico_rand_accu(int source, int pool, uint32_t data);

/* Internal helper functions */
static int pico_rand_reseed(struct pico_rand_generator_state* state, uint8_t* seed, uint8_t seed_size);
static int pico_rand_generate_blocks (struct pico_rand_generator_state* state, uint8_t* buffer, int buffer_size);

/* Get random stuff! */
int pico_rand_bytes(struct pico_rand_generator_state* state, uint8_t* buffer, int count);
int pico_rand_bytes_range(struct pico_rand_generator_state* state, uint8_t* buffer, int count, uint8_t max);

/* Seed persistency functions (if possible on the architecture) */
uint32_t pico_rand_seed_load();
uint32_t pico_rand_seed_store();

/* Shut down the generator securely if it is no longer needed */
void pico_rand_shutdown(struct pico_rand_generator_state* state);

#endif /* PICO_RAND_H_ */

