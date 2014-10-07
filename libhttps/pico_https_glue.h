#ifndef PICO_HTTPS_GLUE_H_
#define PICO_HTTPS_GLUE_H_

#include "pico_https_server.h"
#include "pico_stack.h"
#include "pico_tcp.h"
#include "pico_tree.h"
#include "cyassl/ssl.h"

int pico_EmbedSend(CYASSL* ssl, char *buf, int sz, void *ctx);

int pico_EmbedReceive(CYASSL *ssl, char *buf, int sz, void *ctx);

#endif
