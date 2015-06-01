#include "pico_rand.h"
#include "pico_rand_types.h"

/* Internal helper functions */
static int pico_rand_extract_seed(uint8_t* seed_buffer, int buffer_size);
static int pico_rand_reseed(uint8_t* seed, uint8_t seed_size);
static int pico_rand_generate_block (uint8_t* buffer, int buffer_size);
struct pico_rand_generator_state pico_rand_generator;

struct pico_rand_generator_state pico_rand_generator = {};

/* Sets up the generator, loading the seed from persistent storage if possible*/
int pico_rand_init(void) {
    int i = 0;

    pico_rand_generator.key = PICO_ZALLOC(sizeof(uint8_t) * PICO_RAND_ENCRYPT_KEY_SIZE);
    pico_rand_generator.pool = PICO_ZALLOC(sizeof(Sha256) * PICO_RAND_POOL_COUNT); /* n pools of strings; in practice, hashes iteratively updated */
    pico_rand_generator.counter = PICO_ZALLOC(sizeof(struct pico_rand_counter_fortuna));

    pico_rand_generator.aes = PICO_ZALLOC(sizeof(Aes));
    pico_rand_generator.sha = PICO_ZALLOC(sizeof(Sha256));

    if (NULL == pico_rand_generator.key  ||
        NULL == pico_rand_generator.pool ||
        NULL == pico_rand_generator.aes  ||
        NULL == pico_rand_generator.sha) {
            return -1; /* Failed to allocate memory! */

    }

    /* Set counter to 0 (also indicates unseeded state) */
	pico_rand_init_counter (pico_rand_generator.counter);

    for (i = 0; i < PICO_RAND_POOL_COUNT; i++) {
        wc_InitSha256(&(pico_rand_generator.pool[i])); /* TODO right? What does this look like in assembly? */
    }

	#ifdef PICO_RAND_SEED_PERSISTENT
	    pico_rand_seed_load();

        /* Set up timer for saving seed */

	#endif /* PICO_RAND_SEED_PERSISTENT */

	return 0;

}

/* Add some entropy to a given pool TODO why need source IDs? */
void pico_rand_accu(int source, int pool, uint8_t* data, int data_size) {
    /* To be safe, but should avoid needing to do this, as it'll favour lower pools... */
    pool = pool % PICO_RAND_POOL_COUNT;

    /* Add the new entropy data to the specified pool */
    wc_Sha256Update(&(pico_rand_generator.pool[pool]), data, data_size);

}

/* Prepare a seed from the selected pools. Helper function for re-seeding. */
static int pico_rand_extract_seed(uint8_t* seed_buffer, int buffer_size) {
    int i = 0;
    int hashes_added = 0;

    for (i = 0; i < PICO_RAND_POOL_COUNT; i++) {
        if (((i % 8) == 0) || (pico_rand_generator.counter->values[0] & (1 << ((i - 1) % 8)))) {
            /* Use nth pool every (2^n)th reseed (so p1 every other time, p2 every 4th... */ 
            if ((hashes_added + 1) * PICO_RAND_HASH_SIZE <= buffer_size) {
                 /* Extract final hash for given pool, and put in appropriate part of seed buffer */
                wc_Sha256Final(&(pico_rand_generator.pool[i]), seed_buffer + (PICO_RAND_HASH_SIZE * hashes_added));
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
static int pico_rand_reseed(uint8_t* seed, uint8_t seed_size) {
    uint8_t* sha_input = NULL;

    sha_input = (uint8_t*) PICO_ZALLOC (seed_size + PICO_RAND_ENCRYPT_KEY_SIZE); /* Seed size + 256b of current SHA hash key */

    if (NULL == sha_input) {
        /* Failed to alloc, don't increment counter */
        return 0;

    } else {
        memcpy (sha_input, pico_rand_generator.key, PICO_RAND_ENCRYPT_KEY_SIZE); /* Current key */
        memcpy (sha_input + PICO_RAND_ENCRYPT_KEY_SIZE, seed, seed_size); /* New seed */

    }

    wc_InitSha256(pico_rand_generator.sha);
    wc_Sha256Update(pico_rand_generator.sha, sha_input, seed_size + 32);
    wc_Sha256Final(pico_rand_generator.sha, pico_rand_generator.key);

    pico_rand_increment_counter(pico_rand_generator.counter);

    PICO_FREE (sha_input);

    return 1;

}

/* Generate blocks of random data. Underlying function used by the user-callable random data functions. */
static int pico_rand_generate_block (uint8_t* buffer, int buffer_size) {

    uint8_t encrypt_iv[PICO_RAND_ENCRYPT_IV_SIZE] = { 0 }; /* Can't use ECB mode and combine it with our external counter, so doing it this way */

    /* Run encryption block */
    if (!pico_rand_counter_is_zero (pico_rand_generator.counter)) { /* Refuse if not seeded */
        if (buffer_size >= PICO_RAND_ENCRYPT_BLOCK_SIZE) {
            wc_AesSetKey(pico_rand_generator.aes, pico_rand_generator.key, PICO_RAND_ENCRYPT_KEY_SIZE, encrypt_iv, AES_ENCRYPTION);
            wc_AesCbcEncrypt(pico_rand_generator.aes, buffer, (const byte*) pico_rand_generator.counter, PICO_RAND_ENCRYPT_IV_SIZE);

            pico_rand_increment_counter (pico_rand_generator.counter);

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
int pico_rand_bytes(uint8_t* buffer, int count) {
    int remaining_bytes = 0;
    uint8_t* block_buffer = 0;
    uint8_t* seed_buffer = 0;
    int blocks_done = 0;
    int seed_size = 0;

    if ((count > 0) && (count < (2 * 1024 * 1024))) {

        if ((PICO_TIME_MS() - pico_rand_generator.last_reseed_time >= PICO_RAND_MINIMUM_RESEED_MS) &&
            (1 >= PICO_RAND_MINIMUM_RESEED_ENTR)) { /* FIXME to check 'size' of pool 0 */

            seed_buffer = PICO_ZALLOC (sizeof (uint8_t) * PICO_RAND_POOL_COUNT * PICO_RAND_HASH_SIZE); /* Assume that we use every pool hash... */

            if (NULL == seed_buffer) {
                return 0;

            }

            seed_size = pico_rand_extract_seed (seed_buffer, sizeof (uint8_t) * PICO_RAND_POOL_COUNT * PICO_RAND_HASH_SIZE);
            pico_rand_reseed (seed_buffer, seed_size);
            
            PICO_FREE (seed_buffer);

        }

        /* Get random blocks until we have our data. */
        for (remaining_bytes = count; remaining_bytes > 0; remaining_bytes -= PICO_RAND_ENCRYPT_BLOCK_SIZE) {
            if (remaining_bytes / PICO_RAND_ENCRYPT_BLOCK_SIZE > 0) { /* At least one full block remaining? Can copy directly without overflowing. */
                pico_rand_generate_block(buffer + (PICO_RAND_ENCRYPT_BLOCK_SIZE * blocks_done++), PICO_RAND_ENCRYPT_BLOCK_SIZE); /* TODO check! */

            } else {
                /* This'll only be necessary for the last block, and only if requested byte count != multiple of block size */
                block_buffer = PICO_ZALLOC(PICO_RAND_ENCRYPT_BLOCK_SIZE);

                if (NULL != block_buffer) {
                    pico_rand_generate_block(block_buffer, PICO_RAND_ENCRYPT_BLOCK_SIZE);

                    /* Copy required part of block to output */
                    memcpy (buffer + (blocks_done * PICO_RAND_ENCRYPT_BLOCK_SIZE),
                            block_buffer, remaining_bytes);

                    PICO_FREE (block_buffer);

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

}

/* Get some random bytes within a range. Again, use this instead of modulo. */
int pico_rand_bytes_range(uint8_t* buffer, int count, uint8_t max) { /* TODO */
	return -1; /* FIXME not implemented */

}

/* Wrapper for compatibility with original PRNG */
uint32_t pico_rand(void) {
    uint32_t data;

    pico_rand_bytes((uint8_t*) &data, 4); /* TODO is this fair? */

    return data;

}

/* Seed persistency functions (if possible on the architecture) */
#ifdef PICO_RAND_SEED_PERSISTENT
/* Load seed function */
uint32_t pico_rand_seed_load(void) {
}

/* Save seed function */
uint32_t pico_rand_seed_store(void) {
}
#endif /* PICO_RAND_SEED_PERSISTENT */

void pico_rand_shutdown(void) {
	#ifdef PICO_RAND_SEED_PERSISTENT
	pico_rand_seed_store();
    /* TODO Set up timer for saving seed */
	#endif /* PICO_RAND_SEED_PERSISTENT */

    /* Don't just PICO_FREE them, otherwise generator internals still available in RAM! */
    /* If we're going to set it to something, might as well be 10101010, in case there're any weird electrical effects making it easy to detect was-1s or was-0s or something */
    memset (pico_rand_generator.key, 0x55, sizeof(uint8_t) * 32);
    memset (pico_rand_generator.counter, 0x55, sizeof(struct pico_rand_counter_fortuna));
    memset (pico_rand_generator.pool, 0x55, sizeof(Sha256) * PICO_RAND_POOL_COUNT);
    memset (pico_rand_generator.aes, 0x55, sizeof(Aes));
    memset (pico_rand_generator.sha, 0x55, sizeof(Sha256));

    PICO_FREE(pico_rand_generator.key);
    PICO_FREE(pico_rand_generator.counter);
    PICO_FREE(pico_rand_generator.pool);
    PICO_FREE(pico_rand_generator.aes);
    PICO_FREE(pico_rand_generator.sha);

}

