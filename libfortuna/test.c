#include <stdio.h>
#include "pico_rand.h"

#define PRODUCE_BYTES 100

int main (int argc, char* argv[]) {
    uint8_t iv[16] = { 0 }; /* For the AES, to put it into essentially CBC mode (actually really in counter mode, but using our external counter) */

    uint8_t entropy[32] = {
        0xAA, 0xEE, 0xAB, 0x80, 0xB0, 0x33, 0xA0, 0xBE,
        0xE0, 0xBB, 0xB0, 0x00, 0x31, 0x87, 0x03, 0x91,
        0x45, 0x12, 0x9B, 0x05, 0x70, 0x04, 0xC0, 0xBB,
        0x80, 0x88, 0x99, 0x27, 0x31, 0xE3, 0x4B, 0xB4
    };

    int i;

    uint8_t produced_data[PRODUCE_BYTES] = {0};

    pico_rand_init ();

    /* Entropy tiem */
    for (i = 0; i < 32; i++) {
        pico_rand_accu (i, i, entropy + i, 1);

    }

    int j = pico_rand_bytes (produced_data, PRODUCE_BYTES);

    for (i = 0; i < PRODUCE_BYTES; i++) {
        printf ("%d\n", produced_data[i]);

    }

//    printf ("And uint32_ts with pico_rand...\n");
//    for (i = 0; i < PRODUCE_BYTES / 4; i++) {
//        uint32_t data = pico_rand ();
//        printf ("%lu ", (unsigned long) data);
//
//    }

    pico_rand_shutdown();
    
    return 0;

}
