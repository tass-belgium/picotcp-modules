/*********************************************************************
   PicoTCP. Copyright (c) 2012 TASS Belgium NV. Some rights reserved.
   See LICENSE and COPYING for usage.

   Author: Andrei Carp <andrei.carp@tass.be>
 *********************************************************************/

#ifndef PICO_HTTPS_UTIL_H_
#define PICO_HTTPS_UTIL_H_

/* Informational reponses */
#define HTTPS_CONTINUE                       100u
#define HTTPS_SWITCHING_PROTOCOLS  101u
#define HTTPS_PROCESSING                   102u

/* Success */
#define HTTPS_OK                                     200u
#define HTTPS_CREATED                            201u
#define HTTPS_ACCEPTED                           202u
#define HTTPS_NON_AUTH_INFO              203u
#define HTTPS_NO_CONTENT                     204u
#define HTTPS_RESET_CONTENT              205u
#define HTTPS_PARTIAL_CONTENT            206u
#define HTTPS_MULTI_STATUS                   207u
#define HTTPS_ALREADY_REPORTED           208u
#define HTTPS_LOW_SPACE                      250u
#define HTTPS_IM_SPACE                           226u

/* Redirection */
#define HTTPS_MULTI_CHOICE                   300u
#define HTTPS_MOVED_PERMANENT            301u
#define HTTPS_FOUND                              302u
#define HTTPS_SEE_OTHER                      303u
#define HTTPS_NOT_MODIFIED                   304u
#define HTTPS_USE_PROXY                      305u
#define HTTPS_SWITCH_PROXY                   306u
#define HTTPS_TEMP_REDIRECT              307u
#define HTTPS_PERM_REDIRECT              308u

/* Client error */
#define HTTPS_BAD_REQUEST                    400u
#define HTTPS_UNAUTH                             401u
#define HTTPS_PAYMENT_REQ                    402u
#define HTTPS_FORBIDDEN                      403u
#define HTTPS_NOT_FOUND                      404u
#define HTTPS_METH_NOT_ALLOWED           405u
#define HTTPS_NOT_ACCEPTABLE             406u
#define HTTPS_PROXY_AUTH_REQ             407u
#define HTTPS_REQ_TIMEOUT                    408u
#define HTTPS_CONFLICT                           409u
#define HTTPS_GONE                                   410u
#define HTTPS_LEN_REQ                            411u
#define HTTPS_PRECONDITION_FAIL      412u
#define HTTPS_REQ_ENT_LARGE              413u
#define HTTPS_URI_TOO_LONG                   414u
#define HTTPS_UNSUPORTED_MEDIA           415u
#define HTTPS_REQ_RANGE_NOK              416u
#define HTTPS_EXPECT_FAILED              417u
#define HTTPS_TEAPOT                             418u
#define HTTPS_UNPROC_ENTITY              422u
#define HTTPS_LOCKED                             423u
#define HTTPS_METHOD_FAIL                    424u
#define HTTPS_UNORDERED                      425u
#define HTTPS_UPGRADE_REQ                    426u
#define HTTPS_PRECOND_REQ                    428u
#define HTTPS_TOO_MANY_REQ                   429u
#define HTTPS_HEDER_FIELD_LARGE      431u

/* Server error */
#define HTTPS_INTERNAL_SERVER_ERR    500u
#define HTTPS_NOT_IMPLEMENTED            501u
#define HTTPS_BAD_GATEWAY                    502u
#define HTTPS_SERVICE_UNAVAILABLE    503u
#define HTTPS_GATEWAY_TIMEOUT            504u
#define HTTPS_NOT_SUPPORTED              505u
#define HTTPS_SERV_LOW_STORAGE           507u
#define HTTPS_LOOP_DETECTED              508u
#define HTTPS_NOT_EXTENDED                   510u
#define HTTPS_NETWORK_AUTH                   511u
#define HTTPS_PERMISSION_DENIED      550u

/* Returns used  */
#define HTTPS_RETURN_ERROR       -1
#define HTTPS_RETURN_OK          0
#define HTTPS_RETURN_BUSY        1
#define HTTPS_RETURN_NOT_FOUND   2

/* HTTP Methods */
#define HTTPS_METHOD_GET     1u
#define HTTPS_METHOD_POST    2u

/* List of events - shared between client and server */
#define EV_HTTPS_CON         1u
#define EV_HTTPS_REQ       2u
#define EV_HTTPS_PROGRESS  4u
#define EV_HTTPS_SENT          8u
#define EV_HTTPS_CLOSE     16u
#define EV_HTTPS_ERROR     32u
#define EV_HTTPS_BODY            64u
#define EV_HTTPS_DNS             128u


/* used for chunks */
int pico_itoaHex(uint16_t port, char *ptr);
uint32_t pico_itoa(uint32_t port, char *ptr);
void pico_http_url_decode(char *dst, const char *src);

#endif /* PICO_HTTPS_UTIL_H_ */
