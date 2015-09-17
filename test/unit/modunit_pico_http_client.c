#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "pico_tree.h"
#include "pico_config.h"
#include "pico_socket.h"
#include "pico_tcp.h"
#include "pico_dns_client.h"
#include "pico_http_client.h"
#include "pico_http_util.h"
#include "pico_ipv4.h"
#include "pico_stack.h"

#include "pico_http_client.c"
#include "check.h"

volatile pico_err_t pico_err;

#define RED     0
#define BLACK 1
/* By default the null leafs are black */
struct pico_tree_node LEAF = {
    NULL, /* key */
    &LEAF, &LEAF, &LEAF, /* parent, left,right */
    BLACK, /* color */
};


int pico_dns_client_getaddr(const char *url, void (*callback)(char *ip, void *arg), void *arg)
{
    return 0;
}

int pico_socket_close(struct pico_socket *s)
{
    return 0;
}

int pico_socket_connect(struct pico_socket *s, const void *srv_addr, uint16_t remote_port)
{
    return 0;
}

struct pico_socket *pico_socket_open(uint16_t net, uint16_t proto, void (*wakeup)(uint16_t ev, struct pico_socket *s))
{
    return NULL;
}

int pico_socket_read(struct pico_socket *s, void *buf, int len)
{
    return 0;
}

int pico_socket_write(struct pico_socket *s, const void *buf, int len)
{
    return 0;
}

int pico_string_to_ipv4(const char *ipstr, uint32_t *ip)
{
    return 0;
}

void *pico_tree_delete(struct pico_tree *tree, void *key)
{
    return NULL;
}

void *pico_tree_findKey(struct pico_tree *tree, void *key)
{
    return NULL;
}

struct pico_tree_node *pico_tree_firstNode(struct pico_tree_node *node)
{
    return NULL;
}

void *pico_tree_insert(struct pico_tree *tree, void *key)
{
    return NULL;
}

struct pico_tree_node *pico_tree_next(struct pico_tree_node *node)
{
    return NULL;
}

/* API start */
START_TEST(tc_multipart_chunk_create)
{
    /* TODO: test this: struct multipart_chunk *multipart_chunk_create(uint8_t *data, uint64_t length_data, uint8_t *name, uint8_t *filename, uint8_t *content_disposition, uint8_t *content_type);*/
     
}
END_TEST
/* API end */




START_TEST(tc_free_uri)
{
   /* TODO: test this: static int8_t free_uri(struct pico_http_client *to_be_removed); */
    int ret = 0;
    ret = free_uri(NULL);
    ck_assert_int_eq(ret, HTTP_RETURN_ERROR);
    
    struct pico_http_client *client = NULL;
    client = PICO_ZALLOC(sizeof(struct pico_http_client));
    client->urikey = PICO_ZALLOC(sizeof(struct pico_http_uri));
    ret = free_uri(client);
    ck_assert_int_eq(ret, HTTP_RETURN_OK);
}
END_TEST
START_TEST(tc_client_open)
{
   /* TODO: test this: static int32_t client_open(uint8_t *uri, void (*wakeup)(uint16_t ev, uint16_t conn), int32_t connID); */
    //int ret = 0;
    //ret = client_open(NULL, NULL, 0);
    //ck_assert_int_eq(ret, HTTP_RETURN_ERROR); 
}
END_TEST
START_TEST(tc_free_header)
{
   /* TODO: test this: static void free_header(struct pico_http_client *to_be_removed); */
}
END_TEST
START_TEST(tc_print_request_part_info)
{
   /* TODO: test this: static void print_request_part_info(struct pico_http_client *client) */
}
END_TEST
START_TEST(tc_request_parts_destroy)
{
   /* TODO: test this: static int8_t request_parts_destroy(struct pico_http_client *client) */
    int ret = 0;
    ret = request_parts_destroy(NULL);
    ck_assert_int_eq(ret, HTTP_RETURN_ERROR);
}
END_TEST
START_TEST(tc_socket_write_request_parts)
{
   /* TODO: test this: static int32_t socket_write_request_parts(struct pico_http_client *client) */
}
END_TEST
START_TEST(tc_pico_process_uri)
{
   /* TODO: test this: static int8_t pico_process_uri(const uint8_t *uri, struct pico_http_uri *urikey) */
   int ret = 0;
   ret = pico_process_uri(NULL, NULL);
   ck_assert_int_eq(ret, HTTP_RETURN_ERROR);
   ret = pico_process_uri("blabla", NULL);
   ck_assert_int_eq(ret, HTTP_RETURN_ERROR);
}
END_TEST
START_TEST(tc_compare_clients)
{
   /* TODO: test this: static int32_t compare_clients(void *ka, void *kb) */
}
END_TEST
START_TEST(tc_parse_header_from_server)
{
   /* TODO: test this: static int8_t parse_header_from_server(struct pico_http_client *client, struct pico_http_header *header); */
}
END_TEST
START_TEST(tc_read_chunk_line)
{
   /* TODO: test this: static int8_t read_chunk_line(struct pico_http_client *client); */
}
END_TEST
START_TEST(tc_wait_for_header)
{
   /* TODO: test this: static inline void wait_for_header(struct pico_http_client *client) */
}
END_TEST
START_TEST(tc_treat_write_event)
{
   /* TODO: test this: static void treat_write_event(struct pico_http_client *client) */
}
END_TEST
START_TEST(tc_treat_read_event)
{
   /* TODO: test this: static void treat_read_event(struct pico_http_client *client) */
}
END_TEST
START_TEST(tc_treat_long_polling)
{
   /* TODO: test this: static void treat_long_polling(struct pico_http_client *client, uint16_t ev) */
}
END_TEST
START_TEST(tc_tcp_callback)
{
   /* TODO: test this: static void tcp_callback(uint16_t ev, struct pico_socket *s) */
}
END_TEST
START_TEST(tc_dns_callback)
{
   /* TODO: test this: static void dns_callback(uint8_t *ip, void *ptr) */
}
END_TEST
START_TEST(tc_pico_http_client_build_delete)
{
   /* TODO: test this: static int8_t *pico_http_client_build_delete(const struct pico_http_uri *uri_data, uint8_t connection) */
}
END_TEST
START_TEST(tc_get_content_length)
{
   /* TODO: test this: static int32_t get_content_length(struct multipart_chunk **post_data, uint16_t length, uint8_t *boundary) */
}
END_TEST
START_TEST(tc_get_max_multipart_header_size)
{
   /* TODO: test this: static int32_t get_max_multipart_header_size(struct multipart_chunk **post_data, uint16_t length) */
}
END_TEST
START_TEST(tc_add_multipart_chunks)
{
   /* TODO: test this: static int8_t add_multipart_chunks(struct multipart_chunk **post_data, uint16_t post_data_length, uint8_t *boundary, struct pico_http_client *http) */
}
END_TEST
START_TEST(tc_pico_http_client_build_post_multipart_request)
{
   /* TODO: test this: static int8_t pico_http_client_build_post_multipart_request(const struct pico_http_uri *uri_data, struct multipart_chunk **post_data, uint16_t len, struct pico_http_client *http, uint8_t connection) */
}
END_TEST
START_TEST(tc_pico_http_client_build_post_header)
{
   /* TODO: test this: static int8_t *pico_http_client_build_post_header(const struct pico_http_uri *uri_data, uint32_t post_data_len, uint8_t connection, uint8_t *content_type, uint8_t *cache_control) */
}
END_TEST
START_TEST(tc_check_chunk_line)
{
   /* TODO: test this: static inline int check_chunk_line(struct pico_http_client *client, int tmp_len_read) */
}
END_TEST
START_TEST(tc_update_content_length)
{
   /* TODO: test this: static inline void update_content_length(struct pico_http_client *client, uint32_t tmp_len_read ) */
}
END_TEST
START_TEST(tc_read_body)
{
   /* TODO: test this: static inline int32_t read_body(struct pico_http_client *client, uint8_t *data, uint16_t size, uint32_t *len_read, uint32_t *tmp_len_read) */
}
END_TEST
START_TEST(tc_read_big_chunk)
{
   /* TODO: test this: static inline uint32_t read_big_chunk(struct pico_http_client *client, uint8_t *data, uint16_t size, uint32_t *len_read) */
}
END_TEST
START_TEST(tc_read_small_chunk)
{
   /* TODO: test this: static inline void read_small_chunk(struct pico_http_client *client, uint8_t *data, uint16_t size, uint32_t *len_read) */
}
END_TEST
START_TEST(tc_read_chunked_data)
{
   /* TODO: test this: static inline int32_t read_chunked_data(struct pico_http_client *client, uint8_t *data, uint16_t size) */
}
END_TEST
START_TEST(tc_read_first_line)
{
   /* TODO: test this: static inline void read_first_line(struct pico_http_client *client, uint8_t *line, uint32_t *index) */
}
END_TEST
START_TEST(tc_start_reading_body)
{
   /* TODO: test this: static inline void start_reading_body(struct pico_http_client *client, struct pico_http_header *header) */
}
END_TEST
START_TEST(tc_parse_loc_and_cont)
{
   /* TODO: test this: static inline int32_t parse_loc_and_cont(struct pico_http_client *client, struct pico_http_header *header, uint8_t *line, uint32_t *index) */
}
END_TEST
START_TEST(tc_parse_transfer_encoding)
{
   /* TODO: test this: static inline int32_t parse_transfer_encoding(struct pico_http_client *client, struct pico_http_header *header, uint8_t *line, uint32_t *index) */
}
END_TEST
START_TEST(tc_parse_fields)
{
   /* TODO: test this: static inline int32_t parse_fields(struct pico_http_client *client, struct pico_http_header *header, int8_t *line, uint32_t *index) */
}
END_TEST
START_TEST(tc_parse_rest_of_header)
{
   /* TODO: test this: static inline int32_t parse_rest_of_header(struct pico_http_client *client, struct pico_http_header *header, uint8_t *line, uint32_t *index) */
}
END_TEST
START_TEST(tc_set_client_chunk_state)
{
   /* TODO: test this: static inline void set_client_chunk_state(struct pico_http_client *client) */
}
END_TEST
START_TEST(tc_read_chunk_trail)
{
   /* TODO: test this: static inline void read_chunk_trail(struct pico_http_client *client) */
}
END_TEST
START_TEST(tc_read_chunk_value)
{
   /* TODO: test this: static inline void read_chunk_value(struct pico_http_client *client) */
}
END_TEST


Suite *pico_suite(void)                       
{
    Suite *s = suite_create("PicoTCP-modules HTTPLIB");             

    /*API start*/
    TCase *TCase_multipart_chunk_create = tcase_create("Unit test for tc_multipart_chunk_create");
    
    /*API end*/


    TCase *TCase_free_uri = tcase_create("Unit test for free_uri");
    TCase *TCase_client_open = tcase_create("Unit test for client_open");
    TCase *TCase_free_header = tcase_create("Unit test for free_header");
    TCase *TCase_print_request_part_info = tcase_create("Unit test for print_request_part_info");
    TCase *TCase_request_parts_destroy = tcase_create("Unit test for request_parts_destroy");
    TCase *TCase_socket_write_request_parts = tcase_create("Unit test for socket_write_request_parts");
    TCase *TCase_pico_process_uri = tcase_create("Unit test for pico_process_uri");
    TCase *TCase_compare_clients = tcase_create("Unit test for compare_clients");
    TCase *TCase_parse_header_from_server = tcase_create("Unit test for parse_header_from_server");
    TCase *TCase_read_chunk_line = tcase_create("Unit test for read_chunk_line");
    TCase *TCase_wait_for_header = tcase_create("Unit test for wait_for_header");
    TCase *TCase_treat_write_event = tcase_create("Unit test for treat_write_event");
    TCase *TCase_treat_read_event = tcase_create("Unit test for treat_read_event");
    TCase *TCase_treat_long_polling = tcase_create("Unit test for treat_long_polling");
    TCase *TCase_tcp_callback = tcase_create("Unit test for tcp_callback");
    TCase *TCase_dns_callback = tcase_create("Unit test for dns_callback");
    TCase *TCase_pico_http_client_build_delete = tcase_create("Unit test for *pico_http_client_build_delete");
    TCase *TCase_get_content_length = tcase_create("Unit test for get_content_length");
    TCase *TCase_get_max_multipart_header_size = tcase_create("Unit test for get_max_multipart_header_size");
    TCase *TCase_add_multipart_chunks = tcase_create("Unit test for add_multipart_chunks");
    TCase *TCase_pico_http_client_build_post_multipart_request = tcase_create("Unit test for pico_http_client_build_post_multipart_request");
    TCase *TCase_pico_http_client_build_post_header = tcase_create("Unit test for *pico_http_client_build_post_header");
    TCase *TCase_check_chunk_line = tcase_create("Unit test for check_chunk_line");
    TCase *TCase_update_content_length = tcase_create("Unit test for update_content_length");
    TCase *TCase_read_body = tcase_create("Unit test for read_body");
    TCase *TCase_read_big_chunk = tcase_create("Unit test for read_big_chunk");
    TCase *TCase_read_small_chunk = tcase_create("Unit test for read_small_chunk");
    TCase *TCase_void = tcase_create("Unit test for void");
    TCase *TCase_read_chunked_data = tcase_create("Unit test for read_chunked_data");
    TCase *TCase_read_first_line = tcase_create("Unit test for read_first_line");
    TCase *TCase_start_reading_body = tcase_create("Unit test for start_reading_body");
    TCase *TCase_parse_loc_and_cont = tcase_create("Unit test for parse_loc_and_cont");
    TCase *TCase_parse_transfer_encoding = tcase_create("Unit test for parse_transfer_encoding");
    TCase *TCase_parse_fields = tcase_create("Unit test for parse_fields");
    TCase *TCase_parse_rest_of_header = tcase_create("Unit test for parse_rest_of_header");
    TCase *TCase_set_client_chunk_state = tcase_create("Unit test for set_client_chunk_state");
    TCase *TCase_read_chunk_trail = tcase_create("Unit test for read_chunk_trail");
    TCase *TCase_read_chunk_value = tcase_create("Unit test for read_chunk_value");

    /*API start*/
    tcase_add_test(TCase_multipart_chunk_create, tc_multipart_chunk_create);
    suite_add_tcase(s, TCase_multipart_chunk_create);
    /*API end*/

    tcase_add_test(TCase_free_uri, tc_free_uri);
    suite_add_tcase(s, TCase_free_uri);
    tcase_add_test(TCase_client_open, tc_client_open);
    suite_add_tcase(s, TCase_client_open);
    tcase_add_test(TCase_free_header, tc_free_header);
    suite_add_tcase(s, TCase_free_header);
    tcase_add_test(TCase_print_request_part_info, tc_print_request_part_info);
    suite_add_tcase(s, TCase_print_request_part_info);
    tcase_add_test(TCase_request_parts_destroy, tc_request_parts_destroy);
    suite_add_tcase(s, TCase_request_parts_destroy);
    tcase_add_test(TCase_socket_write_request_parts, tc_socket_write_request_parts);
    suite_add_tcase(s, TCase_socket_write_request_parts);
    tcase_add_test(TCase_pico_process_uri, tc_pico_process_uri);
    suite_add_tcase(s, TCase_pico_process_uri);
    tcase_add_test(TCase_compare_clients, tc_compare_clients);
    suite_add_tcase(s, TCase_compare_clients);
    tcase_add_test(TCase_parse_header_from_server, tc_parse_header_from_server);
    suite_add_tcase(s, TCase_parse_header_from_server);
    tcase_add_test(TCase_read_chunk_line, tc_read_chunk_line);
    suite_add_tcase(s, TCase_read_chunk_line);
    tcase_add_test(TCase_read_body, tc_read_body);
    suite_add_tcase(s, TCase_read_body);
    tcase_add_test(TCase_treat_write_event, tc_treat_write_event);
    suite_add_tcase(s, TCase_treat_write_event);
    tcase_add_test(TCase_treat_read_event, tc_treat_read_event);
    suite_add_tcase(s, TCase_treat_read_event);
    tcase_add_test(TCase_treat_long_polling, tc_treat_long_polling);
    suite_add_tcase(s, TCase_treat_long_polling);
    tcase_add_test(TCase_tcp_callback, tc_tcp_callback);
    suite_add_tcase(s, TCase_tcp_callback);
    tcase_add_test(TCase_dns_callback, tc_dns_callback);
    suite_add_tcase(s, TCase_dns_callback);
    tcase_add_test(TCase_pico_http_client_build_delete, tc_pico_http_client_build_delete);
    suite_add_tcase(s, TCase_pico_http_client_build_delete);
    tcase_add_test(TCase_get_content_length, tc_get_content_length);
    suite_add_tcase(s, TCase_get_content_length);
    tcase_add_test(TCase_get_max_multipart_header_size, tc_get_max_multipart_header_size);
    suite_add_tcase(s, TCase_get_max_multipart_header_size);
    tcase_add_test(TCase_add_multipart_chunks, tc_add_multipart_chunks);
    suite_add_tcase(s, TCase_add_multipart_chunks);
    tcase_add_test(TCase_pico_http_client_build_post_multipart_request, tc_pico_http_client_build_post_multipart_request);
    suite_add_tcase(s, TCase_pico_http_client_build_post_multipart_request);
    tcase_add_test(TCase_pico_http_client_build_post_header, tc_pico_http_client_build_post_header);
    suite_add_tcase(s, TCase_pico_http_client_build_post_header);
    tcase_add_test(TCase_client_open, tc_client_open);
    suite_add_tcase(s, TCase_client_open);
    tcase_add_test(TCase_check_chunk_line, tc_check_chunk_line);
    suite_add_tcase(s, TCase_check_chunk_line);
    tcase_add_test(TCase_update_content_length, tc_update_content_length);
    suite_add_tcase(s, TCase_update_content_length);
    tcase_add_test(TCase_read_big_chunk, tc_read_big_chunk);
    suite_add_tcase(s, TCase_read_big_chunk);
    tcase_add_test(TCase_read_small_chunk, tc_read_small_chunk);
    suite_add_tcase(s, TCase_read_small_chunk);
    tcase_add_test(TCase_read_chunked_data, tc_read_chunked_data);
    suite_add_tcase(s, TCase_read_chunked_data);
    tcase_add_test(TCase_read_first_line, tc_read_first_line);
    suite_add_tcase(s, TCase_read_first_line);
    tcase_add_test(TCase_start_reading_body, tc_start_reading_body);
    suite_add_tcase(s, TCase_start_reading_body);
    tcase_add_test(TCase_parse_loc_and_cont, tc_parse_loc_and_cont);
    suite_add_tcase(s, TCase_parse_loc_and_cont);
    tcase_add_test(TCase_parse_transfer_encoding, tc_parse_transfer_encoding);
    suite_add_tcase(s, TCase_parse_transfer_encoding);
    tcase_add_test(TCase_parse_fields, tc_parse_fields);
    suite_add_tcase(s, TCase_parse_fields);
    tcase_add_test(TCase_parse_rest_of_header, tc_parse_rest_of_header);
    suite_add_tcase(s, TCase_parse_rest_of_header);
    tcase_add_test(TCase_set_client_chunk_state, tc_set_client_chunk_state);
    suite_add_tcase(s, TCase_set_client_chunk_state);
    tcase_add_test(TCase_read_chunk_trail, tc_read_chunk_trail);
    suite_add_tcase(s, TCase_read_chunk_trail);
    tcase_add_test(TCase_read_chunk_value, tc_read_chunk_value);
    suite_add_tcase(s, TCase_read_chunk_value);
return s;
}
                      
int main(void)                      
{                       
    int fails;                      
    Suite *s = pico_suite();                        
    SRunner *sr = srunner_create(s);                        
    srunner_run_all(sr, CK_NORMAL);                     
    fails = srunner_ntests_failed(sr);                      
    srunner_free(sr);                       
    return fails;                       
}
