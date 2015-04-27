#ifndef PICO_WEBSOCKET_UTIL_H
#define PICO_WEBSOCKET_UTIL_H


/* FIN bit -- ==1 if this is the last frame in a message */
#define WS_FIN_ENABLE         1u
#define WS_FIN_DISABLE        0u

/* RSV1, RSV2, RSV3 -- these are one bit each
 * Must be 0 unless an extension is negotiated that defines meanings for non-zero values.
 * If a nonzero value is received and none of the negotiated extensions defines the meanings
 * of such a nonzero value, the receiving endpoint MUST fail the websocket connection
*/

/* List of opcodes - defines operation of the payload data */

#define WS_CONTINUATION_FRAME 0u
#define WS_TEXT_FRAME         1u
#define WS_BINARY_FRAME       2u
#define WS_CONN_CLOSE         8u
#define WS_PING               9u
#define WS_PONG               10u

/* Masking - client has to mask data sent to server */
#define WS_MASK_ENABLE        1u
#define WS_MASK_DISABLE       0u


/* List of events - shared between client and server */
#define EV_WS_ERR             1u
#define EV_WS_BODY            64u

/* Payload lengths, used to determine start of payload data */
#define WS_7_BIT_PAYLOAD_LENGTH_LIMIT          125u
#define WS_16_BIT_PAYLOAD_LENGTH_INDICATOR     126u
#define WS_64_BIT_PAYLOAD_LENGTH_INDICATOR     127u

/* Valid RSV values */
#define RSV_ENABLE            1u
#define RSV_DISABLE           0u

#endif /* PICO_WEBSOCKET_UTIL_H */
