/*********************************************************************
   PicoTCP. Copyright (c) 2012 TASS Belgium NV. Some rights reserved.
   See LICENSE and COPYING for usage.

   Author: Andrei Carp <andrei.carp@tass.be>
 *********************************************************************/


#ifndef PICO_HTTP_CLIENT_H_
#define PICO_HTTP_CLIENT_H_

#include "pico_http_util.h"

/*
 * Transfer encodings
 */
#define HTTP_TRANSFER_CHUNKED  1u
#define HTTP_TRANSFER_FULL     0u

/*
 * Parameters for the send request functions
 */
#define HTTP_CONN_CLOSE         0u
#define HTTP_CONN_KEEP_ALIVE    1u

/*
 * Data types
 */

struct pico_http_header
{
    uint16_t response_code;                  /* http response */
    uint8_t *location;                       /* if redirect is reported */
    uint32_t content_length_or_chunk;        /* size of the message */
    uint8_t transfer_coding;                 /* chunked or full */
};

struct pico_http_client;

struct multipart_chunk
{
    uint8_t *data;
    uint32_t length_data;
    uint8_t *name;
    uint16_t length_name;
    uint8_t *filename;
    uint16_t length_filename;
    uint8_t *content_disposition;
    uint16_t length_content_disposition;
    uint8_t *content_type;
    uint16_t length_content_type;
};

struct multipart_chunk *multipart_chunk_create(uint8_t *data, uint64_t length_data, uint8_t *name, uint8_t *filename, uint8_t *content_disposition, uint8_t *content_type);
int8_t multipart_chunk_destroy(struct multipart_chunk *chunk);
int32_t pico_http_client_get_write_progress(uint16_t conn, uint32_t *total_bytes_written, uint32_t *total_bytes_to_write);
int32_t pico_http_client_open(uint8_t *uri, void (*wakeup)(uint16_t ev, uint16_t conn));
int8_t pico_http_client_send_raw(uint16_t conn, uint8_t *request);
int8_t pico_http_client_send_get(uint16_t conn);
int8_t pico_http_client_send_post(uint16_t conn, uint8_t *post_data, uint32_t post_data_len, uint8_t connection, uint8_t *content_type, uint8_t *cache_control);
int8_t pico_http_client_send_delete(uint16_t conn);
int8_t pico_http_client_send_post_multipart(uint16_t conn, struct multipart_chunk **post_data, uint16_t post_data_len);


struct pico_http_header *pico_http_client_read_header(uint16_t conn);
struct pico_http_uri *pico_http_client_read_uri_data(uint16_t conn);
//int8_t *pico_http_client_build_get(const struct pico_http_uri *uriData);
//int8_t *pico_http_client_build_post_header(const struct pico_http_uri *uriData, uint32_t post_data_len, uint8_t connection, uint8_t *content_type, uint8_t *cache_control);
//int8_t pico_http_client_build_post_multipart_request(const struct pico_http_uri *uriData, struct multipart_chunk **post_data, uint16_t length, struct pico_http_client *http);
//int8_t *pico_http_client_build_delete(const struct pico_http_uri *uriData);

int32_t pico_http_client_read_data(uint16_t conn, uint8_t *data, uint16_t size);
int8_t pico_http_client_close(uint16_t conn);

#endif /* PICO_HTTP_CLIENT_H_ */
