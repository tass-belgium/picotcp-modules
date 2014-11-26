/*********************************************************************
   PicoTCP. Copyright (c) 2012 TASS Belgium NV. Some rights reserved.
   See LICENSE and COPYING for usage.

   Author: Andrei Carp <andrei.carp@tass.be>
 *********************************************************************/

#ifndef PICO_HTTPS_SERVER_H_
#define PICO_HTTPS_SERVER_H_

#include <stdint.h>
#include "pico_https_util.h"
#include "pico_https_glue.h"

/* Response codes */
#define HTTPS_RESOURCE_NOT_FOUND     1u
#define HTTPS_RESOURCE_FOUND         2u
#define HTTPS_STATIC_RESOURCE        4u
#define HTTPS_CACHEABLE_RESOURCE      8u

/* Generic id for the server */
#define HTTPS_SERVER_ID                  0u

/*
 * Server functions
 */
int8_t pico_https_server_start(uint16_t port, void (*wakeup)(uint16_t ev, uint16_t conn));
int pico_https_server_accept(void);
void pico_https_setCertificate(const unsigned char* buffer, int size);
void pico_https_setPrivateKey(const unsigned char* buffer, int size);

/*
 * Client functions
 */
char *pico_https_getResource(uint16_t conn);
int pico_https_getMethod(uint16_t conn);
char *pico_https_getBody(uint16_t conn);
int      pico_https_getProgress(uint16_t conn, uint16_t *sent, uint16_t *total);

/*
 * Handshake and data functions
 */
int      pico_https_respond(uint16_t conn, uint16_t code);
int8_t   pico_https_submitData(uint16_t conn, void *buffer, uint16_t len);
int      pico_https_close(uint16_t conn);

#endif /* PICO_HTTPS_SERVER_H_ */
