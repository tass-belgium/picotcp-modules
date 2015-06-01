/* An implementation of the Fortuna PRNG for PicoTCP */

#ifndef PICO_RAND_H_
#define PICO_RAND_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "wolfssl/wolfcrypt/aes.h"
#include "wolfssl/wolfcrypt/sha256.h"

#include "pico_rand_types.h"
#include "pico_stack.h"

/* Fortuna architecture defines */
#define PICO_RAND_POOL_COUNT            32
#define PICO_RAND_MINIMUM_RESEED_MS     100
#define PICO_RAND_MINIMUM_RESEED_ENTR   1
#define PICO_RAND_MAX_REQUEST_BYTES     1048576

/* Defines for hash and encryption algorithms */
#define PICO_RAND_HASH_SIZE             32
#define PICO_RAND_ENCRYPT_IV_SIZE       16
#define PICO_RAND_ENCRYPT_KEY_SIZE      32
#define PICO_RAND_ENCRYPT_BLOCK_SIZE    16
 
/* Define that we have access to load/store for the seed in this environment. */
// #define PICO_RAND_SEED_PERSISTENT

/* Internal generator state */
struct pico_rand_generator_state {
    /* Fortuna internals */
	uint8_t* key; /* 32 byte key */
	struct pico_rand_counter_fortuna* counter;
	Sha256* pool;
    pico_time last_reseed_time;

    /* AES and SHA internal stuff */
    Aes* aes;
    Sha256* sha;

};
extern struct pico_rand_generator_state pico_rand_generator;

/* For setting up the generator and feeding entropy */
int pico_rand_init(void);
void pico_rand_accu(int source, int pool, uint8_t* data, int data_size);

/* Get random stuff! */
int pico_rand_bytes(uint8_t* buffer, int count);
int pico_rand_bytes_range(uint8_t* buffer, int count, uint8_t max);
uint32_t pico_rand(void);

/* Seed persistency functions (if possible on the architecture) */
#ifdef PICO_RAND_SEED_PERSISTENT
uint32_t pico_rand_seed_load(void);
uint32_t pico_rand_seed_store(void);
#endif

/* Shut down the generator securely if it is no longer needed */
void pico_rand_shutdown(void);

#endif /* PICO_RAND_H_ */

