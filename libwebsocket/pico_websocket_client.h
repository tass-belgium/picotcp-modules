#ifndef PICO_WEBSOCKET_CLIENT_H
#define PICO_WEBSOCKET_CLIENT_H

#include <stdint.h>
#include "pico_websocket_util.h"

int pico_websocket_client_open(char* uri, void (*wakeup)(uint16_t ev, uint16_t conn));
int pico_websocket_client_close(uint16_t connID);

int pico_websocket_client_readData(uint16_t connID, void* data, uint16_t size);
int pico_websocket_client_writeData(uint16_t connID, void* data, uint16_t size);


int pico_websocket_client_add_extension(uint16_t connID, void* extension);
int pico_websocket_client_add_extension(uint16_t connID, void* extension);

int pico_websocket_client_set_RSV_bits(uint16_t connID, uint8_t RSV1, uint8_t RSV2, uint8_t RSV3);

int pico_websocket_client_initiate_connection(uint16_t connID);

#endif /* PICO_WEBSOCKET_CLIENT_H */
