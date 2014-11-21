#include "pico_rand.h"
#include "pico_rand_types.h"


/* Sets up the generator, loading the seed from persistent storage if possible*/
int pico_rand_init(struct pico_rand_generator_state* state) {
    state->key = PICO_ZALLOC(sizeof(uint8_t) * 32);
    state->pool = PICO_ZALLOC(sizeof(uint8_t) * PICO_RAND_POOL_COUNT);

    state->aes = (Aes*) PICO_ZALLOC(sizeof(Aes));
    state->sha = (Sha*) PICO_ZALLOC(sizeof(Sha));

    if (NULL == state->key ||
        NULL == state->pool ||
        NULL == state->aes ||
        NULL == state->sha) {
            return 0; /* Failed to allocate memory! */

    }

    /* Set counter to 0 (also indicates unseeded state) */
	init_counter (state->counter);

	#ifdef PICO_RAND_SEED_PERSISTENT
	    pico_rand_seed_load();

        /* Set up timer for saving seed */

	#endif /* PICO_RAND_SEED_PERSISTENT */

}

/* Add some entropy to a given pool FIXME why need source IDs? */
void pico_rand_accu(int source, int pool, uint32_t data) {
    /* Add the new entropy data to the specified pool */

}

/* Re-seed the generator. Called when enough data has been used from the current 'run' */
static int pico_rand_reseed(struct pico_rand_generator_state* state, uint8_t* seed, uint8_t seed_size) {
    uint8_t* sha_input = NULL;

    sha_input = (uint8_t*) PICO_ZALLOC (seed_size + 32); /* Seed size + 256b of current SHA hash key */

    if (NULL == sha_input) {
        /* Failed to alloc, don't increment counter */
        return 0;

    } else {
        memcpy (sha_input, state->key, 32); /* Current key */
        memcpy (sha_input + 32, seed, seed_size); /* New seed */

    }

    /* Always using one SHA256 hash as seed? What about when more than one pool is to be used for the reseed, should hash together all the pools in question into one? Or re-seed multiple times, once for each pool in question? Or concatenate all hashes together? */

    InitSha(state->sha);
    ShaUpdate(state->sha, sha_input, seed_size + 32);
    ShaFinal(state->sha, state->key);

    increment_counter(state->counter);

    return 1;

}

/* Generate blocks of random data. Underlying function used by the user-callable random data functions. */
static int pico_rand_generate_blocks (struct pico_rand_generator_state* state, uint8_t* buffer, int buffer_size) {
    int i, data;

    uint8_t aes_iv[16] = { 0 }; /* TODO [check] Not used */

    if (!counter_is_zero (state->counter)) { /* Refuse if not seeded */
        for (i = 0; i < buffer_size; i++) {
            AesSetKey(state->aes, state->key, 32, aes_iv, AES_ENCRYPTION);
            AesCbcEncrypt(state->aes, buffer, (const byte*) state->counter, 16);

            *(buffer + i) = data; //FIXME only works for block size 32?
            *(state->counter)++;

        }

        return 1;

    } else {
        /* Not seeded, cannot produce anything! */
        return 0;

    }

}

/* Get some random bytes from the generator. */
int pico_rand_bytes(struct pico_rand_generator_state* state, uint8_t* buffer, int count) {
    int i;

    if (count > 0 && count < 2^20) {
        for (i = 0; i < count; i++) {
            pico_rand_generate_blocks(state, buffer + i, count);

        }

        return 1;

    } else {
        /* Not allowed to produce more than 1MiB of data at once */
        return 0;

    }

}

/* Get some random bytes within a range. Again, use this instead of modulo. */
int pico_rand_bytes_range(struct pico_rand_generator_state* state, uint8_t* buffer, int count, uint8_t max) {
}

/* Load seed function */
uint32_t pico_rand_seed_load() {
}

/* Save seed function */
uint32_t pico_rand_seed_store() {
}

void pico_rand_shutdown(struct pico_rand_generator_state* state) {
	#ifdef PICO_RAND_SEED_PERSISTENT
	    pico_rand_seed_store();

        /* TODO Set up timer for saving seed */

	#endif /* PICO_RAND_SEED_PERSISTENT */

    /* Don't just free them, otherwise generator internals still available in RAM! */
    *(state->key) = 0;
    init_counter (state->counter);
    *(state->pool) = 0;
    //*(state->aes) = 0; /* FIXME */
    //*(state->sha) = 0; /* FIXME */

    free(state->key);
    free(state->counter);
    free(state->pool);
    free(state->aes);
    free(state->sha);

}

