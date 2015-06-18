#ifndef PICO_WEBSOCKET_CLIENT_H
#define PICO_WEBSOCKET_CLIENT_H

#include <stdint.h>
#include "pico_websocket_util.h"

struct pico_websocket_client;
typedef struct pico_websocket_client* WSocket;

WSocket ws_connect(char *uri, char *proto, char *ext);
int ws_read(WSocket ws, void *buf, int len);
int ws_write(WSocket ws, void *buf, int len);
int ws_write_data(WSocket ws, void *buf, int len);
int ws_write_rsv(WSocket ws, void *buf, int len, uint8_t *rsv, uint8_t opcode);
int ws_close(WSocket ws);

#endif /* PICO_WEBSOCKET_CLIENT_H */
