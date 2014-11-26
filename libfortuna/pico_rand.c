#include "pico_rand.h"
#include "pico_rand_types.h"

/* Sets up the generator, loading the seed from persistent storage if possible*/
int pico_rand_init(struct pico_rand_generator_state* state) {
    int i = 0;

    state->key = PICO_ZALLOC(sizeof(uint8_t) * 32);
    state->pool = PICO_ZALLOC(sizeof(Sha256) * PICO_RAND_POOL_COUNT); /* n pools of strings; in practice, hashes iteratively updated */
    state->counter = PICO_ZALLOC(sizeof(struct counter_fortuna));

    state->aes = PICO_ZALLOC(sizeof(Aes));
    state->sha = PICO_ZALLOC(sizeof(Sha256));

    if (NULL == state->key ||
        NULL == state->pool ||
        NULL == state->aes ||
        NULL == state->sha) {
            return 0; /* Failed to allocate memory! */

    }

    /* Set counter to 0 (also indicates unseeded state) */
	init_counter (state->counter);

    for (i = 0; i < PICO_RAND_POOL_COUNT; i++) {
        InitSha256(&(state->pool[i])); // FIXME right? What does this look like in assembly?

    }

	#ifdef PICO_RAND_SEED_PERSISTENT
	    pico_rand_seed_load();

        /* Set up timer for saving seed */

	#endif /* PICO_RAND_SEED_PERSISTENT */

}

/* Add some entropy to a given pool TODO why need source IDs? */
void pico_rand_accu(struct pico_rand_generator_state* state, int source, int pool, uint8_t* data, int data_size) {
    /* To be safe, but should avoid needing to do this, as it'll favour lower pools... */
    pool = pool % PICO_RAND_POOL_COUNT;

    /* Add the new entropy data to the specified pool */
    Sha256Update(&(state->pool[pool]), data, data_size);

}

/* Prepare a seed from the selected pools. Helper function for re-seeding. */
static int pico_rand_extract_seed(struct pico_rand_generator_state* state, uint8_t* seed_buffer, int buffer_size) {
    int i;
    int hashes_added = 0;

    for (i = 0; i < PICO_RAND_POOL_COUNT; i++) {
        if (state->counter->values[i / 8] & 1 << (i % 8)) { /* Use nth pool every (2^n)th reseed (so p0 every time, p1 every other, p2 every 4th... */ // TODO check this logic
            if ((hashes_added + 1) * PICO_RAND_HASH_SIZE <= buffer_size) {
                 /* Extract final hash for given pool, and put in appropriate part of seed buffer */
                Sha256Final(&(state->pool[i]), seed_buffer + (PICO_RAND_HASH_SIZE * hashes_added));
                hashes_added++;

            } else {
                /* Buffer not big enough */
                return 0;

            }

        }

    }

    return (hashes_added * PICO_RAND_HASH_SIZE); /* Actual size of seed */

}

/* Re-seed the generator. Called when enough data has been used from the current 'run' */
static int pico_rand_reseed(struct pico_rand_generator_state* state, uint8_t* seed, uint8_t seed_size) {
    uint8_t* sha_input = NULL;

    sha_input = (uint8_t*) PICO_ZALLOC (seed_size + PICO_RAND_ENCRYPT_KEY_SIZE); /* Seed size + 256b of current SHA hash key */

    if (NULL == sha_input) {
        /* Failed to alloc, don't increment counter */
        return 0;

    } else {
        memcpy (sha_input, state->key, PICO_RAND_ENCRYPT_KEY_SIZE); /* Current key */
        memcpy (sha_input + PICO_RAND_ENCRYPT_KEY_SIZE, seed, seed_size); /* New seed */

    }

    InitSha256(state->sha);
    Sha256Update(state->sha, sha_input, seed_size + 32);
    Sha256Final(state->sha, state->key);

    increment_counter(state->counter);

    free (sha_input);

    return 1;

}

/* Generate blocks of random data. Underlying function used by the user-callable random data functions. */
static int pico_rand_generate_block (struct pico_rand_generator_state* state, uint8_t* buffer, int buffer_size) {

    uint8_t encrypt_iv[PICO_RAND_ENCRYPT_IV_SIZE] = { 0 }; /* Can't use ECB mode and combine it with our external counter, so doing it this way */

    /* Run encryption block */
    if (!counter_is_zero (state->counter)) { /* Refuse if not seeded */
        if (buffer_size >= PICO_RAND_ENCRYPT_BLOCK_SIZE) {
            AesSetKey(state->aes, state->key, PICO_RAND_ENCRYPT_KEY_SIZE, encrypt_iv, AES_ENCRYPTION);
            AesCbcEncrypt(state->aes, buffer, (const byte*) state->counter, PICO_RAND_ENCRYPT_IV_SIZE);

            increment_counter (state->counter);

            return 1; /* Done succesfully */

        }

        /* Buffer size smaller than block size! */
        return 0;

    } else {
        /* Not seeded, cannot produce anything! */
        return 0;

    }

}

/* Get some random bytes from the generator. */
int pico_rand_bytes(struct pico_rand_generator_state* state, uint8_t* buffer, int count) {
    int remaining_bytes = 0;
    uint8_t* block_buffer = 0;
    uint8_t* seed_buffer = 0;
    int blocks_done = 0;
    int seed_size = 0;

    if (count > 0 && count < 2^20) {

        if (1) { // FIXME test time since last reseed, and length(P_0) >= MIN_POOL_SIZE
            seed_buffer = PICO_ZALLOC (sizeof (uint8_t) * PICO_RAND_POOL_COUNT * PICO_RAND_HASH_SIZE); /* Assume that we use every pool hash... */

            seed_size = pico_rand_extract_seed (state, seed_buffer, sizeof (uint8_t) * PICO_RAND_POOL_COUNT * PICO_RAND_HASH_SIZE);
            pico_rand_reseed (state, seed_buffer, seed_size);
            
            free (seed_buffer);

        }

        /* Get random blocks until we have our data. */
        for (remaining_bytes = count; remaining_bytes > 0; remaining_bytes -= PICO_RAND_ENCRYPT_BLOCK_SIZE) {
            if (remaining_bytes / PICO_RAND_ENCRYPT_BLOCK_SIZE > 0) { /* At least one full block remaining? Can copy directly without overflowing. */
                pico_rand_generate_block(state, buffer + (PICO_RAND_ENCRYPT_BLOCK_SIZE * blocks_done++), PICO_RAND_ENCRYPT_BLOCK_SIZE); // TODO check!

            } else {
                /* This'll only happen for the last block, and only if requested byte count != multiple of block size */
                block_buffer = PICO_ZALLOC(PICO_RAND_ENCRYPT_BLOCK_SIZE);

                if (block_buffer) {
                    pico_rand_generate_block(state, block_buffer, PICO_RAND_ENCRYPT_BLOCK_SIZE);

                    /* Copy required part of block to output */
                    memcpy (buffer + (blocks_done * PICO_RAND_ENCRYPT_BLOCK_SIZE),
                            block_buffer, remaining_bytes);

                    free (block_buffer);

                } else {
                    return 0;

                }

            }

        }

        return 1;

    } else {
        /* Not allowed to produce more than 1MiB of data at once */
        return 0;

    }

    // FIXME doesn't return fail if generator not seeded

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
    //*(state->pool) = 0;

    //*(state->aes) = 0; /* FIXME */
    //*(state->sha) = 0; /* FIXME */

    free(state->key);
    free(state->counter);
    // free(state->pool);
    free(state->aes);
    free(state->sha);

}

