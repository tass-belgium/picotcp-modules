#ifndef PICO_HTTPS_GLUE_H_
#define PICO_HTTPS_GLUE_H_

#include "pico_https_server.h"
#include "pico_stack.h"
#include "pico_tcp.h"
#include "pico_tree.h"
#include "polarssl/ssl.h"

int pico_EmbedReceive(void * pico_sock, unsigned char * buf, size_t sz);
int pico_EmbedSend(void * pico_sock, const unsigned char * buf, size_t sz);


#endif
