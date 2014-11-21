#include <stdio.h>
#include "pico_rand.h"

int main (int argc, char* argv[]) {
    static struct pico_rand_generator_state generator;
    static uint8_t iv[16] = { 0 }; /* For the AES, to put it into essentially CBC mode (actually really in counter mode, but using our external counter) */

    const uint8_t entropy[32] = {
        0xAA, 0xEE, 0xAB, 0x80, 0xB0, 0x33, 0xA0, 0xBE,
        0xE0, 0xBB, 0xA0, 0x00, 0x31, 0x87, 0x03, 0x91,
        0x45, 0x12, 0x9B, 0x05, 0x70, 0x04, 0xC0, 0xBB,
        0x80, 0x88, 0x99, 0x27, 0x31, 0xE3, 0x4B, 0xB4
    };

    int i;

    uint8_t produced_data[1024] = {0};

    pico_rand_init (&generator);

    /* Entropy tiem */
    for (i = 0; i < 32; i++) {
        pico_rand_accu (i, i, entropy[i]);

    }

    pico_rand_bytes (&generator, produced_data, 1024);

    for (i = 0; i < 1024; i++) {
        printf ("%d ", produced_data[i]);

    }
    
    return 0;

}
