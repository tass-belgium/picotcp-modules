#include "pico_websocket_client.h"
#include "../libhttp/pico_http_util.h"
#include <stdint.h>
#include <string.h>
#include "pico_tree.h"
#include "pico_config.h"
#include "pico_socket.h"
#include "pico_tcp.h"
#include "pico_ipv4.h"
#include "pico_stack.h"
#include "pico_socket.h"

#define WS_PROTO_TOK                           "ws://"
#define WS_PROTO_LEN                           5u

#define HTTP_HEADER_LINE_SIZE                  50u
#define HTTP_RESPONSE_CODE_INDEX               9u
#define WEBSOCKET_COMMON_HEADER_SIZE           (sizeof(struct pico_websocket_header))

#define PAYLOAD_LENGTH_SIZE_16_BIT_IN_BYTES    2u
#define PAYLOAD_LENGTH_SIZE_64_BIT_IN_BYTES    8u

#define WS_MASKING_KEY_SIZE_IN_BYTES           4u

#define WS_BUFFER_SIZE                         5*1024u /* Provide 5K for fragmented messages (recv or sending) */
#define WS_FRAME_SIZE                          1024u   /* We define one frame as 1K payload */

#define WS_CONTROL_FRAME_MAX_SIZE              125u

#define WS_CLOSING_WAIT_TIME_IN_MS             5000u

#define WS_STATUS_CODE_SIZE_IN_BYTES           2u
#define WS_DEFAULT_STATUS_CODE                 1005u

/* States of the websocket client */
#define WS_INIT                                0u

#define WS_CONNECTING                          1u

#define WS_CONNECTED                           2u

#define WS_CLOSING                             3u
/* I sent a close frame and am waiting for the server to sent his. If WS_CLOSE_PERIOD expires before this happening, I will close the tcp connection */

#define WS_CLOSED                              4u


static uint16_t GlobalWebSocketConnectionID = 0;

/* TODO: provide sending and receive buffer?
 * client could be recv fragmented message and sending fragmented message at the same time?
 */
struct pico_websocket_client
{
        struct pico_socket *sck;
        void (*wakeup)(uint16_t ev, uint16_t conn);
        uint16_t connectionID;
        uint8_t state;
        void* buffer;
        void* next_free_pos_in_buffer;

        struct pico_ip4 ip;
        struct pico_websocket_header* hdr; /* This is the header of the last received frame */
        struct pico_websocket_client_handshake_options* options;
        struct pico_websocket_RSV* rsv;
        struct pico_http_uri* uriKey;
};

PACKED_STRUCT_DEF pico_websocket_header
{
        uint8_t opcode : 4;
        uint8_t RSV3 : 1;
        uint8_t RSV2 : 1;
        uint8_t RSV1 : 1;
        uint8_t fin : 1;
        uint8_t payload_length : 7;
        uint8_t mask : 1;
};

struct pico_websocket_client_handshake_options
{
        /* These members are dynamically allocated and are user-terminated by a '\0' */
        char* protocols;
        char* extensions;
        char* extension_arguments; /* TODO: not yet implemented */
};

/* This struct is used when sending data to the server, the user sets the RSV bits with the function pico_websocket_client_set_RSV_bits() */
struct pico_websocket_RSV
{
        uint8_t RSV3 : 1;
        uint8_t RSV2 : 1;
        uint8_t RSV1 : 1;
};


static void cleanup_websocket_client(pico_time now, void* args);
static int pico_websocket_mask_data(uint32_t masking_key, uint8_t* data, int size);
static int compare_clients_for_same_remote_connection(struct pico_websocket_client* ca, struct pico_websocket_client* cb);
static int determine_payload_length(struct pico_websocket_client* client);
static int compare_ws_with_connID(void *ka, void *kb);
static int is_duplicate_client(struct pico_websocket_client* client);
static int ws_add_protocol_to_client(struct pico_websocket_client* client, void* protocol);
static int ws_add_extension_to_client(struct pico_websocket_client* client, void* extension);
static int check_validity_of_RSV_args(uint8_t RSV1, uint8_t RSV2, uint8_t RSV3);
static struct pico_websocket_header* pico_websocket_client_build_header(struct pico_websocket_RSV* rsv, uint8_t opcode, uint8_t fin, int dataSize);
static int handle_recv_fragmented_frame(struct pico_websocket_client* client);
static int determine_payload_length(struct pico_websocket_client* client);

PICO_TREE_DECLARE(pico_websocket_client_list, compare_ws_with_connID);


/* TODO: move to pico_http_util */
static int8_t pico_process_URI(const char *uri, struct pico_http_uri *urikey)
{

        uint16_t lastIndex = 0, index;

        if(!uri || !urikey || uri[0] == '/')
        {
                pico_err = PICO_ERR_EINVAL;
                goto error;
        }

/* detect protocol => search for  "colon-slash-slash" */
        if(memcmp(uri, WS_PROTO_TOK, WS_PROTO_LEN) == 0) /* could be optimized */
        { /* protocol identified, it is ws */
                urikey->protoHttp = 1;
                lastIndex = WS_PROTO_LEN;
        }

/* detect hostname */
        index = lastIndex;
        while(uri[index] && uri[index] != '/' && uri[index] != ':') index++;
        if(index == lastIndex)
        {
/* wrong format */
                urikey->host = urikey->resource = NULL;
                urikey->port = urikey->protoHttp = 0u;
                pico_err = PICO_ERR_EINVAL;
                goto error;
        }
        else
        {
/* extract host */
                urikey->host = (char *)PICO_ZALLOC((uint32_t)(index - lastIndex + 1));

                if(!urikey->host)
                {
/* no memory */
                        pico_err = PICO_ERR_ENOMEM;
                        goto error;
                }

                memcpy(urikey->host, uri + lastIndex, (size_t)(index - lastIndex));
        }

        if(!uri[index])
        {
/* nothing specified */
                urikey->port = 80u;
                urikey->resource = PICO_ZALLOC(2u);
                if (!urikey->resource) {
/* no memory */
                        pico_err = PICO_ERR_ENOMEM;
                        goto error;
                }

                urikey->resource[0] = '/';
                return 0;
        }
        else if(uri[index] == '/')
        {
                urikey->port = 80u;
        }
        else if(uri[index] == ':')
        {
                urikey->port = 0u;
                index++;
                while(uri[index] && uri[index] != '/')
                {
/* should check if every component is a digit */
                        urikey->port = (uint16_t)(urikey->port * 10 + (uri[index] - '0'));
                        index++;
                }
        }

/* extract resource */
        if(!uri[index])
        {
                urikey->resource = PICO_ZALLOC(2u);
                if (!urikey->resource) {
/* no memory */
                        pico_err = PICO_ERR_ENOMEM;
                        goto error;
                }

                urikey->resource[0] = '/';
        }
        else
        {
                lastIndex = index;
                while(uri[index]) index++;
                urikey->resource = (char *)PICO_ZALLOC((size_t)(index - lastIndex + 1));

                if(!urikey->resource)
                {
/* no memory */
                        pico_err = PICO_ERR_ENOMEM;
                        goto error;
                }

                memcpy(urikey->resource, uri + lastIndex, (size_t)(index - lastIndex));
        }

        return 0;

error:
        if(urikey->resource)
        {
                PICO_FREE(urikey->resource);
                urikey->resource = NULL;
        }

        if(urikey->host)
        {
                PICO_FREE(urikey->host);
                urikey->host = NULL;
        }

        return -1;
}

/* TODO: this function is copied from pico_http_util.c, make http_util.c a common library or something to avoid code duplication */
uint32_t pico_itoa(uint32_t port, char *ptr)
{
        uint32_t size = 0;
        uint32_t index;

        /* transform to from number to string [ in backwards ] */
        while(port)
        {
                ptr[size] = (char)(port % 10 + '0');
                port = port / 10;
                size++;
        }
        /* invert positions */
        for(index = 0; index < (size >> 1u); index++)
        {
                char c = ptr[index];
                ptr[index] = ptr[size - index - 1];
                ptr[size - index - 1] = c;
        }
        ptr[size] = '\0';
        return size;
}

static int is_duplicate_client(struct pico_websocket_client* client)
{
        struct pico_tree_node *index;

        if (!client)
        {
                return -1;
        }

        pico_tree_foreach(index, &pico_websocket_client_list)
        {
                if (((struct pico_websocket_client *)index->keyValue) == client)
                {
                        continue;
                }

                if (compare_clients_for_same_remote_connection(client, ((struct pico_websocket_client *)index->keyValue)) == 0)
                {
                        return 0;
                        break;
                }
        }

        return -1;
}


static int compare_clients_for_same_remote_connection(struct pico_websocket_client* ca, struct pico_websocket_client* cb)
{
        if (ca->ip.addr == cb->ip.addr )
        {
                if (ca->uriKey->port == cb->uriKey->port)
                {
                        return 0;
                }
        }
        return -1;
}


static int compare_ws_with_connID(void *ka, void *kb)
{
        return ((struct pico_websocket_client *)ka)->connectionID - ((struct pico_websocket_client *)kb)->connectionID;
}


static struct pico_websocket_client* retrieve_websocket_client_with_conn_ID(uint16_t wsConnID)
{
        struct pico_websocket_client dummy = {
                .connectionID = wsConnID
        };
        struct pico_websocket_client *client = pico_tree_findKey(&pico_websocket_client_list, &dummy);

        if(!client)
        {
                dbg("Wrong connection id !\n");
                pico_err = PICO_ERR_EINVAL;
                return NULL;
        }

        return client;

}

static struct pico_websocket_client* find_websocket_client_with_socket(struct pico_socket* s)
{
        struct pico_websocket_client *client = NULL;
        struct pico_tree_node *index;

        if (!s)
        {
                return NULL;
        }

        pico_tree_foreach(index, &pico_websocket_client_list)
        {
                if(((struct pico_websocket_client *)index->keyValue)->sck == s )
                {
                        client = (struct pico_websocket_client *)index->keyValue;
                        break;
                }
        }

        if(!client)
        {
                return NULL;
        }

        return client;

}

/* For the add_*_to_client functions, allocate more memory for new protocol/extension + insert terminator string at the end of the array '\0' */
static int ws_add_protocol_to_client(struct pico_websocket_client* client, void* protocol)
{
        /* TODO */
        return 0;
}

static int ws_add_extension_to_client(struct pico_websocket_client* client, void* extension)
{
        /* TODO */
        return 0;
}

static int check_validity_of_RSV_arg(uint8_t RSV)
{
        if (RSV != RSV_ENABLE && RSV != RSV_DISABLE)
        {
                return -1;
        }

        return 0;
}

static int check_validity_of_RSV_args(uint8_t RSV1, uint8_t RSV2, uint8_t RSV3)
{
        return check_validity_of_RSV_arg(RSV1) + check_validity_of_RSV_arg(RSV2) + check_validity_of_RSV_arg(RSV3);
}


/* If close frame has a payload, the first two bytes in the payload are an unisgned int in network order. This is why we add WS_STATUS_CODE_SIZE_IN_BYTES to the reason_size when checking the size. */
static int send_close_frame(struct pico_websocket_client* client, uint16_t status_code, void* reason, uint8_t reason_size)
{
        int ret;
        uint32_t masking_key;
        struct pico_websocket_header* hdr = PICO_ZALLOC(sizeof(struct pico_websocket_header));

        hdr->opcode = WS_CONN_CLOSE;
        hdr->fin = WS_FIN_ENABLE;
        hdr->RSV1 = 0;
        hdr->RSV2 = 0;
        hdr->RSV3 = 0;
        hdr->mask = WS_MASK_ENABLE;
        masking_key = pico_rand();

        if (reason_size + WS_STATUS_CODE_SIZE_IN_BYTES > WS_CONTROL_FRAME_MAX_SIZE)
        {
                dbg("Size of reason is too big to fit in close frame.\n");
                return -1;
        }

        if (reason && reason_size)
        {
                hdr->payload_length = reason_size + WS_STATUS_CODE_SIZE_IN_BYTES;
        }

        ret = pico_socket_write(client->sck, hdr, sizeof(struct pico_websocket_header));

        if (ret < 0)
        {
                dbg("Failed to write header for close frame.\n");
                return -1;
        }

        ret = pico_socket_write(client->sck, &masking_key, WS_MASKING_KEY_SIZE_IN_BYTES);

        if (ret < 0)
        {
                dbg("Failed to write header for close frame.\n");
                return -1;
        }

        if (reason && reason_size)
        {
                /* When no status code is provided, the server will default to 1005, so not our responsibility */
                if (status_code)
                {
                        status_code = short_be(status_code);
                        status_code = pico_websocket_mask_data(masking_key, &status_code, WS_STATUS_CODE_SIZE_IN_BYTES );
                        ret = pico_socket_write(client->sck, &status_code, WS_STATUS_CODE_SIZE_IN_BYTES);
                }
                reason = pico_websocket_mask_data(masking_key, reason, reason_size);
                ret = pico_socket_write(client->sck, reason, reason_size);
        }


        /* client->state = WS_CLOSING; */

        return ret;
}


static int fail_websocket_connection(struct pico_websocket_client* client)
{
        switch (client->state)
        {
        case WS_CONNECTED:
                /* I recv a close frame, I should sent one back. Now I am closed*/
                send_close_frame(client, 0, NULL, 0);
                client->state = WS_CLOSED;
                pico_timer_add(WS_CLOSING_WAIT_TIME_IN_MS, cleanup_websocket_client, (void*)client);
                break;
        case WS_CLOSING:
                /* I sent a close frame and am waiting for the servers response */
                pico_timer_add(WS_CLOSING_WAIT_TIME_IN_MS, cleanup_websocket_client, (void*)client);
                break;
        case WS_CLOSED:
                break;
        default:
                pico_timer_add(WS_CLOSING_WAIT_TIME_IN_MS, cleanup_websocket_client, (void*)client);
                break;
        }

        return 0;
}



static int pico_websocket_client_cleanup(struct pico_websocket_client* client)
{
        struct pico_websocket_client *toBeRemoved = NULL;
        int ret;

        dbg("Closing the websocket client...\n");

        toBeRemoved = pico_tree_delete(&pico_websocket_client_list, client);
        if(!toBeRemoved)
        {
                dbg("Warning ! Websocket to be closed not found ...\n");
                return -1;
        }

        if (client->sck)
        {
                ret = pico_socket_close(client->sck);
                if (ret < 0)
                {
                        dbg("Unable to close socket of websocket client.\n");
                        return -1;
                }
        }

        if (client->options)
        {
                if (client->options->extensions)
                {
                        PICO_FREE(client->options->extensions);
                }

                if (client->options->protocols)
                {
                        PICO_FREE(client->options->protocols);
                }

                if (client->options->extension_arguments )
                {
                        PICO_FREE(client->options->extension_arguments);
                }
        }

        if (client->hdr)
        {
                PICO_FREE(client->hdr);
        }

        if (client->uriKey)
        {
                if (client->uriKey->host)
                {
                        PICO_FREE(client->uriKey->host);
                }

                if (client->uriKey->resource)
                {
                        PICO_FREE(client->uriKey->resource);
                }
                PICO_FREE(client->uriKey);
        }

        if (client->buffer)
        {
                PICO_FREE(client->buffer);
        }

        PICO_FREE(client);

        dbg("Client was succesfully closed.\n");
        return 0;
}

static void cleanup_websocket_client(pico_time now, void* args)
{
        struct pico_websocket_client* client = (struct pico_websocket_client*) args;

        pico_websocket_client_cleanup(client);
}


static int pico_websocket_mask_data(uint32_t masking_key, uint8_t* data, int size)
{
        int i;
        uint8_t mask[4];

        memcpy(mask, &masking_key, WS_MASKING_KEY_SIZE_IN_BYTES);

        for( i = 0 ; i < size ; ++i )
        {
                data[i] = data[i] ^ mask[i%4];
        }

        return i;
}

static int handle_recv_pong_frame(struct pico_websocket_client* client)
{
        /* When a pong frame is received, a reply is not mandatory. */
        return 0;
}

/* We received a ping frame, we should respond with a pong frame (with same payload data if ping frame has payload data) */
static int handle_recv_ping_frame(struct pico_websocket_client* client)
{
        struct pico_websocket_header* hdr = client->hdr;
        uint64_t payload_length = determine_payload_length(client);
        uint8_t* recv_payload;
        uint32_t masking_key;
        int ret;

        if (payload_length > WS_CONTROL_FRAME_MAX_SIZE)
        {
                /* Received a ping frame with a payload length greater than 125 bytes, fail connection */
                fail_websocket_connection(client);
                return -1;
        }

        if ( hdr->mask == WS_MASK_ENABLE )
        {
                /* Ping frame from server should not be masked, fail connection */
                fail_websocket_connection(client);
                return -1;
        }

        struct pico_websocket_header* pong_header= pico_websocket_client_build_header(NULL, WS_PONG, WS_FIN_ENABLE, payload_length);
        masking_key = pico_rand();

        recv_payload = PICO_ZALLOC(payload_length);

        ret = pico_socket_read(client->sck, recv_payload, payload_length);

        /* TODO: pass data to user! */

        pico_websocket_mask_data(masking_key, recv_payload, payload_length);

        ret = pico_socket_write(client->sck, pong_header, sizeof(struct pico_websocket_header))
                + pico_socket_write(client->sck, &masking_key, WS_MASKING_KEY_SIZE_IN_BYTES )
                + pico_socket_write(client->sck, recv_payload, payload_length);

        if (ret < 0)
        {
                dbg("Failed to sent pong frame in response to ping frame.\n");

        }

        PICO_FREE(recv_payload);

        return ret;
}

static int handle_recv_close_frame(struct pico_websocket_client* client)
{
        struct pico_websocket_header* hdr = client->hdr;
        uint16_t status_code;
        int ret;
        uint8_t* reason = PICO_ZALLOC(WS_CONTROL_FRAME_MAX_SIZE);

        if (hdr->payload_length)
        {
                /* Server has supplied a reason for closing the connection */

                /* Check if frame is valid */
                if (hdr->payload_length > WS_CONTROL_FRAME_MAX_SIZE)
                {
                        dbg("Received a close frame that was longer than allowed.\nFailing the conn.\n");
                        return -1;
                }

                /* First 2 bytes are an unsigned int in network order that represents the status code */
                pico_socket_read(client->sck, &status_code, 2);
                status_code = short_be(status_code);

                dbg("Received close frame with status code : %u\n", status_code);

                /* Extract reason from body */
                ret = pico_socket_read(client->sck, reason, WS_CONTROL_FRAME_MAX_SIZE);

                if (ret < 0)
                {
                        dbg("Reading reason of close frame failed.\n");
                        return -1;
                }

                reason[ret]= (char)0;
                dbg("And reason: %s\n", reason);
        }

        PICO_FREE(reason);

        fail_websocket_connection(client);

        return 0;
}


static inline int read_char_from_client_socket(struct pico_websocket_client* client, char* output)
{
        if (!client)
        {
                return -1;
        }
        return pico_socket_read(client->sck, output, 1u);
}

/* Note: "\r\n" is not considered a line, so the last line of a http message will give back strlen(line) == 0 */
static void ws_read_http_header_line(struct pico_websocket_client* client, char *line)
{
        char c;
        uint32_t index = 0;

        while(read_char_from_client_socket(client, &c) > 0 && c != '\r')
        {
                line[index++] = c;
        }
        line[index] = (char)0;
        read_char_from_client_socket(client, &c); /* Consume '\n' */
}


static void* allocate_client_buffer(struct pico_websocket_client* client)
{
        if (!client->buffer)
        {
                client->buffer = PICO_ZALLOC(WS_BUFFER_SIZE);
                client->next_free_pos_in_buffer = client->buffer;
        }

        return client->buffer;
}

static int handle_recv_text_frame(struct pico_websocket_client* client)
{
        if ( client->hdr->fin == WS_FIN_DISABLE )
        {
                handle_recv_fragmented_frame(client);
        }
        else
        {
                client->wakeup(EV_WS_BODY, client->connectionID);
        }
        return 0;
}

static int frame_is_control_frame(struct pico_websocket_client* client)
{
        struct pico_websocket_header* hdr = client->hdr;

        if (hdr->opcode != WS_CONN_CLOSE || hdr->opcode != WS_PING || hdr->opcode != WS_PONG)
        {
                return -1;
        }

        return 0;
}

/* TODO: handle buffer overflow better, maybe let client consume what we can store and clear it afterwards, then receive the rest */
static int handle_recv_fragmented_frame(struct pico_websocket_client* client)
{
        struct pico_websocket_header* hdr = client->hdr;
        int ret, remaining_size_buffer;
        uint64_t payload_length;


        if (frame_is_control_frame(client) == 0)
        {
                /* Control frame should not be fragmented, fail the connection */
                fail_websocket_connection(client);
                return -1;
        }

        payload_length = determine_payload_length(client);

        if (payload_length < 0)
        {
                dbg("Could not determine payload length.\n");
                return -1;
        }

        allocate_client_buffer(client);
        remaining_size_buffer = WS_BUFFER_SIZE - (client->next_free_pos_in_buffer - client->buffer);

        if (payload_length > remaining_size_buffer)
        {
                /* Message is too big for the remaining buffer, truncate data */

                ret = pico_socket_read(client->sck, client->next_free_pos_in_buffer, remaining_size_buffer);

                uint8_t* throw_away = PICO_ZALLOC(payload_length - remaining_size_buffer);
                /* Truncate data (find better way, see todo above)*/
                pico_socket_read(client->sck, throw_away, payload_length - remaining_size_buffer);

                PICO_FREE(throw_away);

                /* Update the next free pos, this will be used in readData() */
                client->next_free_pos_in_buffer += ret;

                /* Wakeup the client to let him consume the fragmented messages */
                client->wakeup(EV_WS_BODY, client->connectionID);
        }
        else
        {
                ret = pico_socket_read(client->sck, client->next_free_pos_in_buffer, payload_length);
                if ( hdr->opcode == 0 && hdr->fin == WS_FIN_ENABLE )
                {
                        /* opcode = 0 and fin = 1 mean this is the last packed, so we can fr
                           ee the buffer after delivering the data to the user. */
                        client->wakeup(EV_WS_BODY, client->connectionID);

                        PICO_FREE(client->buffer);
                        client->next_free_pos_in_buffer = NULL;
                }

        }

        if (ret < 0)
        {
                /* TODO: free buffer here? */
                return -1;
        }

        return ret;
}

static int parse_websocket_header(struct pico_websocket_client* client)
{
        int ret;
        uint8_t opcode;
        uint8_t* start_of_data;
        uint64_t payload_size;
        uint32_t masking_key;

        struct pico_websocket_header* hdr = client->hdr;

        if (hdr->mask == WS_MASK_ENABLE)
        {
                /* Server should not mask data, fail the connection */
                fail_websocket_connection(client);
        }

        /* Opcode */
        switch(hdr->opcode)
        {
        case WS_CONTINUATION_FRAME:
                opcode = WS_CONTINUATION_FRAME;
                /* TODO: reenable */
                /* handle_recv_fragmented_frame(client); */
                break;
        case WS_CONN_CLOSE:
                opcode = WS_CONN_CLOSE;
                handle_recv_close_frame(client);
                break;
        case WS_PING:
                opcode = WS_PING;
                handle_recv_ping_frame(client);
                break;
        case WS_PONG:
                opcode = WS_PONG;
                break;
        case WS_BINARY_FRAME:
                opcode = WS_BINARY_FRAME;
                break;
        case WS_TEXT_FRAME:
                opcode = WS_TEXT_FRAME;
                handle_recv_text_frame(client);
                break;
        default:
                /*bad header*/
                ret = -1;
                break;
        }

        return ret;
}

static void handle_websocket_message(struct pico_websocket_client* client)
{
        int ret;

        /* Read the comon header part */
        ret = pico_socket_read(client->sck, client->hdr , WEBSOCKET_COMMON_HEADER_SIZE);
        if (ret < 0)
        {
                client->wakeup(EV_WS_ERR, client->connectionID);
                return;
        }

        if (ret == 0)
        {
                /* Nothing received, so nothing to parse */
                return;
        }

        /* Parse the header using the common part */
        ret = parse_websocket_header(client);
}


static void add_protocols_to_header(char* header, char* protocols)
{
        if (!protocols)
        {
                /* No protocols specified by user, defaulting to chat and superchat */
                strcat(header, "Sec-WebSocket-Protocol: chat, superchat\r\n");
                return;
        }

        while(*protocols != '\0')
        {
                strcat(header, "Sec-WebSocket-Protocol: ");
                strcat(header, protocols);
                if (*(protocols+1) != '\0')
                {
                        strcat(header, ", ");
                }

                protocols++;
        }
        strcat(header, "\r\n");
}

static void add_extensions_to_header(char* header, char* extensions)
{
        if (!extensions)
        {
                /* No extensions specified by user, defaulting to no extensions */
                return;
        }

        while(*extensions != '\0')
        {
                strcat(header, "Sec-WebSocket-Extensions: ");
                strcat(header, extensions);
                if (*(extensions+1) != '\0')
                {
                        strcat(header, ", ");
                }

                extensions++;
        }
        strcat(header, "\r\n");
}


static char* build_pico_websocket_upgradeHeader(struct pico_websocket_client* client)
{
        /* TODO: review this building process */
        char* header;
        int header_size = 256;
        struct pico_http_uri* uriKey = client->uriKey;


        header = PICO_ZALLOC(header_size);
        strcpy(header, "GET /chat HTTP/1.1\r\n");
        strcat(header, "Host: ");
        strcat(header, uriKey->host);

        if (uriKey->port)
        {
                char port[6u];
                pico_itoa(uriKey->port, port);
                strcat(header, ":");
                strcat(header, port);
        }
        strcat(header, "\r\n");
        strcat(header, "Upgrade: websocket\r\n");
        strcat(header, "Connection: Upgrade\r\n");
        strcat(header, "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"); /*TODO: this is a 16-byte value that has been base64-encoded, it is randomly selected. */

        add_protocols_to_header(header, client->options->protocols);
        add_extensions_to_header(header, client->options->extensions);

        strcat(header, "Sec-WebSocket-Version: 13\r\n");
        strcat(header, "\r\n");

        return header;
}

/* Mask bit is not in parameter list because client always has to mask */
static struct pico_websocket_header* pico_websocket_client_build_header(struct pico_websocket_RSV* rsv, uint8_t opcode, uint8_t fin, int dataSize)
{
        struct pico_websocket_header * header = PICO_ZALLOC(sizeof(struct pico_websocket_header));

        if (!header)
        {
                dbg("Could not allocate websocket header.\n");
                return NULL;
        }


        if (!rsv)
        {
                header->RSV1 = 0;
                header->RSV2 = 0;
                header->RSV3 = 0;
        }
        else
        {
                header->RSV1 = rsv->RSV1;
                header->RSV2 = rsv->RSV2;
                header->RSV3 = rsv->RSV3;
        }
        header->mask = WS_MASK_ENABLE;

        header->opcode = opcode;

        header->fin = fin;
        header->payload_length = dataSize;

        return header;
}

static int pico_websocket_client_send_upgrade_header(struct pico_websocket_client* client)
{
        char *header;
        int ret;
        struct pico_socket* socket = client->sck;

        header= build_pico_websocket_upgradeHeader(client);
        if (!header)
        {
                dbg("WebSocket Header could not be created.\n");
                return -1;
        }

        ret = pico_socket_write(socket, header, strlen(header));

        if(ret < 0)
        {
                dbg("Failed to send upgrade header.\n");
                return -1;
        }

        return ret;
}


static int parse_server_http_respone_line(char* line, char* colon_ptr)
{
        /* TODO */
        /* Don't forgot to check for the pico_websocket_client_handshake options */
        return 0;
}

static int ws_parse_upgrade_header(struct pico_websocket_client* client)
{
        char* line = PICO_ZALLOC(HTTP_HEADER_LINE_SIZE);
        char* colon_ptr;
        uint16_t responseCode = -1;
        int ret;

        ws_read_http_header_line(client, line);

        responseCode = (uint16_t)((line[HTTP_RESPONSE_CODE_INDEX] - '0') * 100 +
                                  (line[HTTP_RESPONSE_CODE_INDEX + 1] - '0') * 10 +
                                  (line[HTTP_RESPONSE_CODE_INDEX + 2] - '0'));

        switch (responseCode)
        {
        case HTTP_SWITCHING_PROTOCOLS:
                dbg("Response to upgrade request was HTTP_SWITCHING_PROTOCOLS (101).\n");
                break;
        case HTTP_UNAUTH:
                /* TODO */
                return -1;
        default:
                if (responseCode >= HTTP_MULTI_CHOICE && responseCode < HTTP_BAD_REQUEST)
                {
                        /* REDIRECTION */
                        /* TODO */
                        return -1;
                }
                return -1;
        }

        /* Response code was OK, validate rest of the response */
        do{
                ws_read_http_header_line(client, line);
                colon_ptr = strstr(line, ":");
                if (colon_ptr != NULL)
                {
                        ret = parse_server_http_respone_line(line, colon_ptr);
                        if (ret < 0)
                        {
                                /* TODO: cancel everything */
                        }
                }
        } while ( strlen(line) != 0 );

        return 0;
}

static void ws_treat_read_event(struct pico_websocket_client* client)
{
        dbg("treat read event, client state: %d\n", client->state);
        if (client->state == WS_INIT)
        {
                dbg("Client is still in init state! Can't handle read event now.\n");
                return;
        }

        if (client->state == WS_CONNECTING)
        {
                int ret;
                dbg("Parse upgrade header sent by back server.\n");

                ret = ws_parse_upgrade_header(client);

                if (ret < 0)
                {
                        dbg("received bad header from server.\n");
                        /* TODO: close everything */
                        return;
                }

                client->state = WS_CONNECTED;

                /* Allocate header for future recv messages */
                client->hdr = PICO_ZALLOC(sizeof(struct pico_websocket_header));

                if(!client->hdr)
                {
                        dbg("Out of memory to allocate websocket header.\n");
                        pico_err = PICO_ERR_ENOMEM;
                        return;
                }


                return;
        }

        if (client->state == WS_CONNECTED)
        {

                handle_websocket_message(client);
                return;
        }

        if (client->state == WS_CLOSING)
        {
                return;
        }
}


static void ws_tcp_callback(uint16_t ev, struct pico_socket *s)
{
        int ret;
        struct pico_websocket_client* client = find_websocket_client_with_socket(s);

        if(!client)
        {
                return;
        }


        if(ev & PICO_SOCK_EV_CONN)
        {
                ret = pico_websocket_client_send_upgrade_header(client);
                if (ret < 0)
                {
                        dbg("Webserver client sendHeader failed.\n");
                        pico_websocket_client_close(client->connectionID);
                        return;
                }
        }

        if(ev & PICO_SOCK_EV_ERR)
        {
                dbg("Error happened with socket.\n");
                fail_websocket_connection(client);
        }

        if(ev & PICO_SOCK_EV_CLOSE)
        {
                dbg("Close socket received.\n");
                pico_socket_close(client->sck);
        }

        if(ev & PICO_SOCK_EV_FIN)
        {
                dbg("FIN received.\n");
        }

        if(ev & PICO_SOCK_EV_RD)
        {
                ws_treat_read_event(client);
        }

        if(ev & PICO_SOCK_EV_WR)
        {
                /* ? */
        }
}



/* used for getting a response from DNS servers */
static void dnsCallback(char *ip, void *ptr)
{
        struct pico_websocket_client *client = (struct pico_websocket_client *)ptr;

        if(!client)
        {
                dbg("Who made the request ?!\n");
                return;
        }

        if(ip)
        {
                pico_string_to_ipv4(ip, &client->ip.addr);
        }
        else
        {
                /* wakeup client and let know error occured */
                client->wakeup(EV_WS_ERR, client->connectionID);

                /* close the client (free used heap) */
                pico_websocket_client_close(client->connectionID);
        }
}



static struct pico_websocket_client* build_pico_websocket_client_with_callback(void(*wakeup)(uint16_t ev, uint16_t conn))
{
        /* TODO: solve memory leaks with a cleanup function */
        struct pico_websocket_client *client = PICO_ZALLOC(sizeof(struct pico_websocket_client));

        if (!client)
        {
                dbg("Could not allocate websocket client.\n");
                return NULL;
        }

        client->wakeup = wakeup;
        client->connectionID = GlobalWebSocketConnectionID++;
        client->state = WS_INIT;
        client->uriKey = PICO_ZALLOC(sizeof(struct pico_http_uri));

        if(!client->uriKey)
        {
                dbg("Failed to allocate urikey for websocket client.\n");
                return NULL;
        }

        client->options = PICO_ZALLOC(sizeof(struct pico_http_uri));
        if(!client->options)
        {
                dbg("failed to allocate options structure for websocket client.\n");
                return NULL;
        }

        if(pico_tree_insert(&pico_websocket_client_list, client))
        {
                /* already in : should not be possible*/
                pico_err = PICO_ERR_EEXIST;
                /* TODO: cleanup! */
                return NULL;
        }

        return client;
}


static int determine_payload_length(struct pico_websocket_client* client)
{
        int ret;
        uint8_t payload_length = client->hdr->payload_length;

        if (client == NULL)
        {
                return -1;
        }

        switch(payload_length)
        {
        case WS_16_BIT_PAYLOAD_LENGTH_INDICATOR:
                ret = pico_socket_read(client->sck, &payload_length, PAYLOAD_LENGTH_SIZE_16_BIT_IN_BYTES);
                break;
        case WS_64_BIT_PAYLOAD_LENGTH_INDICATOR:
                ret = pico_socket_read(client->sck, &payload_length, PAYLOAD_LENGTH_SIZE_64_BIT_IN_BYTES);
                break;
        default:
                /* No extra reading required, size is already known */
                break;
        }

        if (ret < 0)
        {
                return -1;
        }

        return payload_length;
}


/**
 * This function will open a new websocket client.
 * To contact the websocket server, you have to call pico_websocket_client_initiate_connection().
 * Format of the uri must be the following: "ws:" "//" host [ ":" port ] path [ "?" query ] .
 * Events that can occur: EV_WS_ERR and EV_WS_BODY.
 *
 * @param uri This is uri of the server you want to contact.
 * @param wakeup This is the callback function which will be called if an event occurs.
 * @return Will return < 0 if an error occured. Will return the connection ID of the client if succesfull.
 */
int pico_websocket_client_open(char* uri, void (*wakeup)(uint16_t ev, uint16_t conn))
{
        uint32_t ip = 0;
        int ret;
        struct pico_websocket_client* client;

        client = build_pico_websocket_client_with_callback(wakeup);

        if (client == NULL)
        {
                dbg("Client could not be allocated!\n");
                return -1;
        }

        ret = pico_process_URI(uri, client->uriKey);

        if (ret < 0)
        {
                dbg("URI could not be processed.\n");
                return -1;
        }

        /* dns query */
        if(pico_string_to_ipv4(client->uriKey->host, &ip) == -1)
        {
                dbg("Querying : %s \n", client->uriKey->host);
                pico_dns_client_getaddr(client->uriKey->host, dnsCallback, client);
        }
        else
        {
                dbg("host already and ip address, no dns required.\n");
                dnsCallback(client->uriKey->host, client);
        }

        return client->connectionID;
}

/**
 * This function will close the websocket client.
 * Close messages will be sent before cleaning up the client.
 *
 * @param connID This is the connection ID of the websocket client.
 * @return Will return < 0 if an error occured. Will return 0 if succesfull.
 */
int pico_websocket_client_close(uint16_t connID)
{
        int ret;
        struct pico_websocket_client *client;

        client = retrieve_websocket_client_with_conn_ID(connID);

        if (!client)
        {
                dbg("Websocket client cannot be closed, wrong connID provided!\n");
                return -1;
        }

        return fail_websocket_connection(client);
}

/**
 * This function will read data received from the websocket server.
 * This function can be called after receiving a WS_EV_BODY in your callback provided in pico_websocket_client_open().
 * If size is smaller than the data received, the data will be truncated.
 *
 * @param connID This is the connection ID of the websocket client.
 * @param data This is the buffer that will be used to hold your data.
 * @param size This is the size of the data you want to read.
 * @return Will return < 0 if an error occured. Will return number of bytes read if succesfull.
 */
int pico_websocket_client_readData(uint16_t connID, void* data, uint16_t size)
{
        int ret;
        int payload_length;

        struct pico_websocket_client* client = retrieve_websocket_client_with_conn_ID(connID);

        if (!client)
        {
                dbg("Wrong conn ID for readData!\n");
                return;
        }

        if (client->buffer)
        {
                /* We were called after receiving fragmented frames */
                int size_of_data_in_buffer = client->next_free_pos_in_buffer - client->buffer;

                if (size < size_of_data_in_buffer)
                {
                        dbg("User provided a buffer which is too small. Truncate data.\n");
                        memcpy(data, client->buffer, size);
                        return size;
                }
                memcpy(data, client->buffer, size_of_data_in_buffer);

                /* Free the websocket client buffer */
                PICO_FREE(client->buffer);
                /* Reset the next free pos in buffer, just to be sure */
                client->next_free_pos_in_buffer = NULL;

                return size_of_data_in_buffer;
        }

        /* We were called after receiving a normal message */

        struct pico_websocket_header* hdr = client->hdr;

        payload_length = determine_payload_length(client);

        if (payload_length < 0)
        {
                dbg("Could not determine payload length.\n");
                return -1;
        }

        if (payload_length > size)
        {
                dbg("Warning, the size you want to read is too small. Data will be truncated.\n");
                ret = pico_socket_read(client->sck, data, size);
        }
        else
        {
                ret = pico_socket_read(client->sck, data, payload_length);
        }

        return ret;
}


/**
 * This function will sent data to the websocket server.
 * If size is greater than 1024 bytes, the message will be fragmented.
 *
 * @param connID This is the connection ID of the websocket client.
 * @param data This is the data you want to send.
 * @param size This is the size of the data you want to send.
 * @return Will return < 0 if an error occured. Will return number of bytes sent if succesfull.
 */
int pico_websocket_client_writeData(uint16_t connID, void* data, uint16_t size)
{
        int ret;
        struct pico_websocket_client* client = retrieve_websocket_client_with_conn_ID(connID);
        struct pico_socket* socket = client->sck;
        uint32_t masking_key = pico_rand();
        struct pico_websocket_header* header;
        uint8_t header_payload_indicator;
        uint16_t size_network_order = short_be(size);

        if (!client)
        {
                dbg("Wrong connID provided to writeData.\n");
                return -1;
        }

        /* Data will fit in one frame */
        if ( size <= WS_FRAME_SIZE)
        {
                if (size > WS_7_BIT_PAYLOAD_LENGTH_LIMIT)
                {
                        header_payload_indicator = WS_16_BIT_PAYLOAD_LENGTH_INDICATOR;
                }
                else
                {
                        header_payload_indicator = size;
                }

                header = pico_websocket_client_build_header(client->rsv, WS_TEXT_FRAME, WS_FIN_ENABLE, header_payload_indicator);
                masking_key = pico_rand();
                pico_websocket_mask_data(masking_key, (uint8_t*)data, size);

                ret = pico_socket_write(socket, header, sizeof(struct pico_websocket_header));
                ret = header_payload_indicator == size ? 0 : pico_socket_write(socket, &size_network_order, PAYLOAD_LENGTH_SIZE_16_BIT_IN_BYTES);
                ret = pico_socket_write(socket, &masking_key, WS_MASKING_KEY_SIZE_IN_BYTES);
                ret = pico_socket_write(socket, data, size);

                PICO_FREE(header);
                return ret;
        }

        /* Data will not fit in one frame, fragment it */
        while(1)
        {
                /* TODO: what to do with RSV bits? and extension data*/
                header = pico_websocket_client_build_header(client->rsv, WS_TEXT_FRAME, WS_FIN_DISABLE, WS_FRAME_SIZE);
                masking_key = pico_rand();
                pico_websocket_mask_data(masking_key, (uint8_t*)data, WS_FRAME_SIZE);

                ret = pico_socket_write(socket, header, sizeof(struct pico_websocket_header))
                        + pico_socket_write(socket, &masking_key, WS_MASKING_KEY_SIZE_IN_BYTES )
                        + pico_socket_write(socket, data, size);

                if (ret < 0 )
                {
                        return ret;
                }

                data += WS_FRAME_SIZE;
                size -= WS_FRAME_SIZE;

                /* Is size an exact multiple of WS_FRAME_SIZE? If so, we need another header, see below */
                if (size == WS_FRAME_SIZE)
                {
                        break;
                }

                /* Will size fit in one packet? if so, we need another header, see below */
                if (size < WS_FRAME_SIZE)
                {
                        break;
                }

                PICO_FREE(header);
        }

        /* Send out last packet, it has to have a different header compared to the previous fragmented ones*/
        header = pico_websocket_client_build_header(client->rsv, WS_CONTINUATION_FRAME, WS_FIN_ENABLE, size);
        masking_key = pico_rand();
        pico_websocket_mask_data(masking_key, (uint8_t*)data, size);

        ret = pico_socket_write(socket, header, sizeof(struct pico_websocket_header))
                + pico_socket_write(socket, &masking_key, WS_MASKING_KEY_SIZE_IN_BYTES )
                + pico_socket_write(socket, data, size);

        PICO_FREE(header);

        return ret;
}

/**
 * This will add a protocol to the websocket client.
 * This will be added to the upgrade header sent to the server.
 * If no protocols are provided, we will default to "/chat"
 *
 * @param connID This is the connection ID of the websocket client.
 * @param protocol This is the protocol you want to negotiate with the server. This will most likely be a char array describing the extension (for example "/chat")
 * @return Will return < 0 if an error occured. Will return 0 if succesfull.
 */
int pico_websocket_client_add_protocol(uint16_t connID, void* protocol)
{
        struct pico_websocket_client* client = retrieve_websocket_client_with_conn_ID(connID);

        if (!client)
        {
                dbg("Websocket client cannot be found, wrong connID provided!\n");
                return -1;
        }

        if (client->state != WS_INIT)
        {
                dbg("Protocols have to be added before initiating handshake!\n");
                return -1;
        }

        if (ws_add_protocol_to_client(client, protocol) < 0)
        {
                dbg("Could not add protol to client.\n");
                return -1;
        }

        return 0;
}

/**
 * This will add an extension to the websocket client.
 * This will be added to the upgrade header sent to the server.
 * If no extensions are provided, we will default to "" (empty string)
 *
 * @param connID This is the connection ID of the websocket client.
 * @param extension This is the extension you want to negotiate with the server. This will most likely be a char array describing the extension.
 * @return Will return < 0 if an error occured. Will return 0 if succesfull.
 */
int pico_websocket_client_add_extension(uint16_t connID, void* extension)
{
        struct pico_websocket_client* client = retrieve_websocket_client_with_conn_ID(connID);

        if (!client)
        {
                dbg("Websocket client cannot be found, wrong connID provided!\n");
                return -1;
        }

        if (client->state != WS_INIT)
        {
                dbg("Extensions have to be added before initiating handshake!\n");
                return -1;
        }

        if (ws_add_extension_to_client(client, extension) < 0)
        {
                dbg("Could not add protol to client.\n");
                return -1;
        }

        return 0;

}

/**
 * This function will initiate the handshake with the websocket server.
 *
 * @param connID This is the connection ID you received with pico_websocket_client_open().
 * @return Will return < 0 if an error occured. Will return 0 if succesfull.
 */
int pico_websocket_client_initiate_connection(uint16_t connID)
{
        struct pico_websocket_client* client = retrieve_websocket_client_with_conn_ID(connID);
        int ret;


        if (!client)
        {
                dbg("Websocket client cannot be initiated, wrong connID provided!\n");
                return -1;
        }

        /* TODO: check if dns ready */

        if (is_duplicate_client(client) == 0)
        {
                dbg("You can only open one websocket connection to the same IP adress.\n");
                return -1;
        }

        if (client->state != WS_INIT)
        {
                dbg("Client is already started or failed to connect earlier.\n");
        }

        client->sck = pico_socket_open(PICO_PROTO_IPV4, PICO_PROTO_TCP, &ws_tcp_callback);
        if(!client->sck)
        {
                dbg("Failed to open socket.\n");
                client->wakeup(EV_WS_ERR, client->connectionID);
                return;
        }

        if(pico_socket_connect(client->sck, &client->ip, short_be(client->uriKey->port)) < 0)
        {
                client->wakeup(EV_WS_ERR, client->connectionID);
                return;
        }

        client->state = WS_CONNECTING;

        return 0;
}


/**
 * This will set the RSV bits for the following message you send out using pico_websocket_client_writeData(). If you do not provide RSV bits using this function, pico_websocket_client_writeData() will default to RSV1 = RSV2 = RSV3 = 0.
 *
 * @param RSV1 This is the RSV1 bit in the websocket header
 * @param RSV2 This is the RSV2 bit in the websocket header
 * @param RSV3 This is the RSV3 bit in the websocket header
 * @return Will return < 0 if an error occured. Will return 0 if succesfull.
 */
int pico_websocket_client_set_RSV_bits(uint16_t connID, uint8_t RSV1, uint8_t RSV2, uint8_t RSV3)
{
        struct pico_websocket_client* client = retrieve_websocket_client_with_conn_ID(connID);
        int ret;

        if (!client)
        {
                dbg("Websocket client cannot be found, wrong connID provided!\n");
                return -1;
        }

        ret = check_validity_of_RSV_args(RSV1, RSV2, RSV3);

        if (ret < 0)
        {
                dbg("One of the provided RSV args is not valid! Please use RSV_ENABLE/DISABLE to set/unset the RSV bit.\n");
                return -1;
        }

        client->rsv = PICO_ZALLOC(sizeof(struct pico_websocket_RSV));

        if (!client->rsv)
        {
                dbg("Could not allocate rsv struct.\n");
                return -1;
        }

        /* Set the RSV args of the new struct */
        client->rsv->RSV1 = RSV1;
        client->rsv->RSV2 = RSV2;
        client->rsv->RSV3 = RSV3;

        return 0;
}
