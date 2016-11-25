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

#include "encoding.c"
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

/* MOCKS */
static struct pico_socket example_socket;
static struct pico_http_client *example_client = NULL;
static int write_success_cnt = 0;
static int write_progress_made_cnt = 0;
static int write_in_chunks = 0;
static int header_ev_cnt = 0;
static int body_ev_cnt = 0;
static int read_header_in_chunks = 0;
static int clear_read_idx = 0;
static int chunked_response = 0;
static int test_404_with_body = 0;
static int test_404_without_body = 0;

/*static inline void *pico_zalloc(size_t size)
{
    void *ptr = malloc(size);
    if(ptr)
        memset(ptr, 0u, size);
    return ptr;
}*/

void cb(uint16_t ev, uint16_t conn)
{
    printf("Callback! %d\n", ev);
    if (ev & EV_HTTP_REQ)
    {
        printf("Read header event\n");
        header_ev_cnt++;
    }
    if (ev & EV_HTTP_BODY)
    {
        printf("Read body event\n");
        body_ev_cnt++;
    }
    if (ev & EV_HTTP_WRITE_SUCCESS)
    {
        printf("Write success event\n");
        write_success_cnt++;
    }
    if (ev & EV_HTTP_WRITE_PROGRESS_MADE)
    {
        printf("Write progress made\n");
        write_progress_made_cnt++;
    }
}

int pico_dns_client_getaddr(const char *url, void (*callback)(char *ip, void *arg), void *arg)
{
    return 0;
}

int pico_socket_close(struct pico_socket *s)
{
    fail_if(s != &example_socket);
    return 0;
}

int pico_socket_connect(struct pico_socket *s, const void *srv_addr, uint16_t remote_port)
{
    printf("pico_socket_connect %p, %p\n", s, &example_socket);
    fail_if(s != &example_socket);
    return 0;
}

int pico_socket_setoption(struct pico_socket *s, int option, void *value)
{
    return 0;
}

struct pico_socket *pico_socket_open(uint16_t net, uint16_t proto, void (*wakeup)(uint16_t ev, struct pico_socket *s))
{
    example_socket.wakeup = wakeup;
    return &example_socket;
}

int pico_socket_read(struct pico_socket *s, void *buf, int len)
{
    char response[1024];
    if (chunked_response)
    {
        strcpy(response,"HTTP/1.1 200 OK\r\nServer: BaseHTTP/0.3 Python/2.7.6\r\nDate: Fri, 09 Oct 2015 11:26:38 GMT\r\nTransfer-Encoding: chunked\r\nContent-type: text/plain\r\n\r\n12\r\nthis is chunk: 0\r\n\r\n12\r\nthis is chunk: 1\r\n\r\n12\r\nthis is chunk: 2\r\n\r\n12\r\nthis is chunk: 3\r\n\r\n12\r\nthis is chunk: 4\r\n\r\n12\r\nthis is chunk: 5\r\n\r\n12\r\nthis is chunk: 6\r\n\r\n12\r\nthis is chunk: 7\r\n\r\n12\r\nthis is chunk: 8\r\n\r\n12\r\nthis is chunk: 9\r\n\r\n0");
    }
    else if(test_404_with_body)
    {
        strcpy(response, "HTTP/1.1 404 NOT FOUND\r\nServer: nginx\r\nDate: Mon, 02 Nov 2015 09:17:10 GMT\r\nContent-Type: text/html\r\nContent-Length: 233\r\nConnection: close\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Credentials: true\r\n\r\n<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n<title>404 Not Found</title>\r\n<h1>Not Found</h1>\r\n<p>The requested URL was not found on the server.  If you entered the URL manually please check your spelling and try again.</p>\r\n");
    }
    else if(test_404_without_body)
    {
    }
    else
    {
        //strcpy(response, "HTTP/1.1 200 get balbalba\r\nContent-Length: 12\r\nServer: BaseHTTP/0.3 Python/2.7.6\r\nDate: Thu, 01 Oct 2015 08:12:05 GMT\r\n\r\nget balbalba");
        strcpy(response, "HTTP/1.1 200\r\nContent-Length: 36\r\n\r\n{\"Colour\":\"green\", \"Flash\":\"FSHING\"}");
    }
    int length = strlen(response);
    static int idx = 0;
    int bytes_read = 0;
    //printf("in pico_socket_read length: %d\n", length);
    if(clear_read_idx)
    {
        idx = 0;
        clear_read_idx = 0;
    }

    if (read_header_in_chunks && idx == 10)
    {
        read_header_in_chunks = 0;
        return 0;
    }

    if (length == idx)
    {
        return 0;
    }
    else if (len < (length - idx))
    {
        //printf("buf: %p, response: %p, idx: %d, response + idx: %p, len: %d\n", buf,response, idx, &response[idx], len);
        memcpy((char *)buf, &response[idx], len);
        //printf("buf: %c\n", ((char *)buf)[0]);
        idx += len;
    }
    else
    {
        bytes_read = length-idx;
        //printf("length-idx %d-%d = %d   buf: %p\n", length, idx, bytes_read, buf);
        memcpy((char *)buf, (char *)&response[idx], bytes_read);
        //printf("return %d\n", bytes_read);
        idx += bytes_read;
        return bytes_read;
    }
    fail_if(s != &example_socket);
    //printf("Return len: %d\n", len);
    return len;
}

int pico_socket_write(struct pico_socket *s, const void *buf, int len)
{
    fail_if(buf == NULL);
    fail_if(len == 0);
    printf("pico_socket_write %p, %p, buf: %p len: %d\n", s, &example_socket, buf, len);
    fail_if(s != &example_socket);
    if (write_in_chunks)
    {
        write_in_chunks = 0;
        return (len/2);
    }
    else
    {
        return len;
    }
}

int pico_string_to_ipv4(const char *ipstr, uint32_t *ip)
{
    return 0;
}

void *pico_tree_delete(struct pico_tree *tree, void *key)
{

    if (((struct pico_http_client*)key)->connectionID == 99)
    {
        return NULL;
    }
    return example_client;
}

void *pico_tree_findKey(struct pico_tree *tree, void *key)
{
    if (((struct pico_http_client*)key)->connectionID == 99)
    {
        return NULL;
    }
    return example_client;
}

struct pico_tree_node *pico_tree_firstNode(struct pico_tree_node *node)
{
    return NULL;
}

void *pico_tree_insert(struct pico_tree *tree, void *key)
{
    example_client = key;
    return NULL;
}

struct pico_tree_node *pico_tree_next(struct pico_tree_node *node)
{
    return NULL;
}


/* API start */
START_TEST(tc_multipart_chunk_create)
{
    /* TODO: test this: struct multipart_chunk *multipart_chunk_create(uint8_t *data, uint64_t length_data, char *name, char *filename, char *content_disposition, char *content_type);*/
    uint8_t data[10] = "data";
    char name[10] = "test";
    char filename[10] = "test.txt";
    char cont_disp[20] = "content_disp_test";
    char cont_type[15] = "cont_t_test";
    struct multipart_chunk *mpch = NULL;
    printf("\n\nStart: tc_multipart_chunk_create\n");

    /*Case1: data is NULL-ptr*/
    mpch = multipart_chunk_create(NULL, 4, name, filename, cont_disp, cont_type);
    fail_if(mpch != NULL);
    multipart_chunk_destroy(mpch);
    /*Case2: data length is 0*/
    mpch = multipart_chunk_create(data, 0, name, filename, cont_disp, cont_type);
    fail_if(mpch != NULL);
    multipart_chunk_destroy(mpch);
    /*Case3: possitive test, check if everting got assigned*/
    mpch = multipart_chunk_create(data, 4, name, filename, cont_disp, cont_type);
    fail_if(mpch == NULL);
    fail_if(strncmp(mpch->name, name, strlen(name)) != 0, "Error name in chunk");
    fail_if(mpch->length_name != strlen(name));
    fail_if(strncmp(mpch->filename, filename, strlen(filename)) != 0, "Error filename in chunk");
    fail_if(mpch->length_filename != strlen(filename));
    fail_if(strncmp(mpch->content_disposition, cont_disp, strlen(cont_disp)) != 0, "Error cont_disp in chunk");
    fail_if(mpch->length_content_disposition != strlen(cont_disp));
    fail_if(strncmp(mpch->content_type, cont_type, strlen(cont_type)) != 0, "Error cont_type in chunk");
    fail_if(mpch->length_content_type != strlen(cont_type));
    fail_if(mpch->length_data != 4);
    multipart_chunk_destroy(mpch);
    printf("Stop: tc_multipart_chunk_create\n");
}
END_TEST
START_TEST(tc_multipart_chunk_destroy)
{
    /* TODO: test this: int8_t multipart_chunk_destroy(struct multipart_chunk *chunk);*/
    struct multipart_chunk *mpch = NULL;
    int8_t ret = 0;
    printf("\n\nStart: tc_multipart_chunk_destroy\n");
    mpch = multipart_chunk_create((uint8_t *)"data", 4, "name", "filename", "cont_disp", "cont_type");
    /*Case1: free NULL-ptr*/
    ret = multipart_chunk_destroy(NULL);
    fail_if(ret != -1);
    /*Case2: positive test*/
    ret = multipart_chunk_destroy(mpch);
    fail_if(ret != 0);
    printf("Stop: tc_multipart_chunk_destroy\n");
}
END_TEST
START_TEST(tc_pico_process_uri)
    {
        // Format : http://<user>:<pwd>@<host>:<port>/<path>/
        printf("\n\nStart: tc_pico_process_uri\n");
        struct pico_http_uri *urikey;
        urikey = PICO_ZALLOC(sizeof(struct pico_http_uri));
        int32_t conn = 0;
        char uri[50] = "http://user:pwd@httpbin.org:8080/";
        // Case 1: No urikey
        conn = pico_process_uri("blabla", NULL, NULL);
        fail_if(conn != HTTP_RETURN_ERROR);
        // Case 2: No uri
        conn = pico_process_uri(NULL, NULL, NULL);
        fail_if(conn != HTTP_RETURN_ERROR);
        // Case 3: unknown protocol (no :// in hostname)
        conn = pico_process_uri("//test.org/", urikey, NULL);
        fail_if(conn > 0);
        urikey = PICO_ZALLOC(sizeof(struct pico_http_uri));
        // Case 4: No <host>
        conn = pico_process_uri("http://user:pwd@:80/", urikey, NULL);
        fail_if(conn != HTTP_RETURN_ERROR);
        urikey = PICO_ZALLOC(sizeof(struct pico_http_uri));
        // Case 5: No <port> if there is :
        conn = pico_process_uri("http://user:pwd@httpbin:/", urikey, NULL);
        fail_if(conn != HTTP_RETURN_ERROR);
        urikey = PICO_ZALLOC(sizeof(struct pico_http_uri));
        // Case 6: No host and port
        conn = pico_process_uri("http://user:pwd@/", urikey, NULL);
        fail_if(conn != HTTP_RETURN_ERROR);
        urikey = PICO_ZALLOC(sizeof(struct pico_http_uri));
        // Case 7: port is not a digit
        conn = pico_process_uri("http://user:pwd@httpbin.org:54notadigit43/", urikey, NULL);
        fail_if(conn != HTTP_RETURN_ERROR);
        urikey = PICO_ZALLOC(sizeof(struct pico_http_uri));
        // Case 8: positive test
        conn = pico_process_uri(uri, urikey, NULL);
        fail_if(conn < 0);
        PICO_FREE(urikey);
        urikey = NULL;
        printf("Stop: tc_pico_process_uri\n");
    }
END_TEST
START_TEST(tc_base64_encode)
{
    printf("\n\nStart: tc_base64_encode\n");
    char *userpass[] = {"user:pass\n", "panda:isalive\n", "rabbit:eatcarrots\n"};
    char *encodes[] = {"dXNlcjpwYXNz\n", "cGFuZGE6aXNhbGl2ZQ==\n", "cmFiYml0OmVhdGNhcnJvdHM=\n"};

    int i;
    for (i = 0; i < 3; i++){
        char buffout[256];

	int error = encode(buffout, userpass[i]);
	fail_if(error < 0);

        printf("Real value : %s\nGet : %s\n\n", encodes[i], buffout);
        fail_if(strcmp(encodes[i], buffout) != 0);
    }
    printf("Stop: tc_base64_encode\n");

    printf("\n\nStart: tc_pico_http_client_open_with_usr_pwd_encoding\n");

    int32_t conn = 0;
    char uri[50] = "http://user:pass@httpbin.org:8080/";

    conn = pico_http_client_open_with_usr_pwd_encoding(uri, cb, encode);
    fail_if(conn < 0);
    pico_http_client_close(conn);

    printf("Stop: tc_pico_http_client_open_with_usr_pwd_encoding\n");

}
END_TEST
START_TEST(tc_pico_http_client_open)
{
    //TODO: test this: int32_t pico_http_client_open(char *uri, void (*wakeup)(uint16_t ev, uint16_t conn));
    int32_t conn = 0;
    char uri[50] = "http://user:pwd@httpbin.org:8080/";
    printf("\n\nStart: tc_pico_http_client_open\n");
    /*Case1: no callback*/
    conn = pico_http_client_open(uri, NULL);
    fail_if(conn != HTTP_RETURN_ERROR);
    /*Case2: no uri*/
    conn = pico_http_client_open(NULL, cb);
    fail_if(conn != HTTP_RETURN_ERROR);
    /*Case3: positive test*/
    conn = pico_http_client_open(uri, cb);
    fail_if(conn < 0);
    pico_http_client_close(conn);
    printf("Stop: tc_pico_http_client_open\n");
}
END_TEST
START_TEST(tc_pico_http_client_send_raw)
{
    //TODO: test this: int8_t pico_http_client_send_raw(uint16_t conn, char *request);
    int32_t ret = 0;
    int16_t conn = 0;
    char uri[50] = "http://user:pwd@httpbin.org/";
    char request[256] = "POST / HTTP/1.1\nUser-Agent: picotcp\nAccept: */*\nHost: 192.168.2.22:8080\nConnection: Keep-Alive\nContent-Type: application/x-www-form-urlencoded\nContent-Length: 17\n\nkey=1&test=robbin";
    printf("\n\nStart: tc_pico_http_client_send_raw\n");
    /*Case1: unknown client*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_raw(99, request);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    //Case2: empty request
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_raw(conn, NULL);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    //Case3: positive test, everything got written at once
    write_success_cnt = 0;
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_raw(conn, request);
    fail_if(ret != HTTP_RETURN_OK);
    fail_if(write_success_cnt != 1);
    pico_http_client_close(conn);
    //Case4: positive test, write in 2chunks
    write_success_cnt = 0;
    write_in_chunks = 1;
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_raw(conn, request);
    fail_if(ret != HTTP_RETURN_OK);
    //tread write event zou eigenlijk getriggerd moeten worden door de tcp callback
    treat_write_event(example_client);
    //tcp_callback(PICO_SOCK_EV_WR, &example_socket);
    fail_if(write_progress_made_cnt != 1);
    fail_if(write_success_cnt != 1);
    pico_http_client_close(conn);
    printf("Stop: tc_pico_http_client_send_raw\n");
}
END_TEST
START_TEST(tc_pico_http_client_send_get)
{
    //TODO: test this: int8_t pico_http_client_send_get(uint16_t conn, char *resource);
    int32_t ret = 0;
    int16_t conn = 0;
    char uri[50] = "http://user:pwd@httpbin.org/";
    printf("\n\nStart: tc_pico_http_client_send_get\n");
    /*Case1: unknown client*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_get(99, "/", HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    /*Case2: unknown connection*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_get(conn, "/", 99);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    /*Case3: positive test, everything got written at once*/
    write_success_cnt = 0;
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_get(conn, "/", HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_OK);
    fail_if(write_success_cnt != 1);
    pico_http_client_close(conn);
    /*Case4: empty resource*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_get(conn, NULL, HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_OK);
    pico_http_client_close(conn);
    printf("Stop: tc_pico_http_client_send_get\n");
}
END_TEST
START_TEST(tc_pico_http_client_send_post)
{
    //TODO: test this: int8_t pico_http_client_send_post(uint16_t conn, char *resource, uint8_t *post_data, uint32_t post_data_len, uint8_t connection, char *content_type, char *cache_control);
    int32_t ret = 0;
    int16_t conn = 0;
    unsigned char *post_data = (unsigned char*)"key=1&robbin=robbin";
    uint32_t post_data_len = strlen((char*)post_data);
    //char content_type[50] = "";
    //char cache_controle[50] = "";
    char uri[50] = "http://user:pwd@httpbin.org/";
    printf("\n\nStart: tc_pico_http_client_send_post\n");
    /*Case1: unknown client*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post(99, "/", post_data, post_data_len, HTTP_CONN_CLOSE, NULL, NULL);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    /*Case2: empty data*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post(conn, "/", NULL, post_data_len, HTTP_CONN_CLOSE, NULL, NULL);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    /*Case3: data_len = 0*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post(conn, "/", post_data, 0, HTTP_CONN_CLOSE, NULL, NULL);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    /*Case4: unknown connection*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post(conn, "/", post_data, post_data_len, 100, NULL, NULL);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    /*Case5: positive test, everything got written at once*/
    write_success_cnt = 0;
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post(conn, "/", post_data, post_data_len, HTTP_CONN_CLOSE, NULL, NULL);
    fail_if(ret != HTTP_RETURN_OK);
    fail_if(write_success_cnt != 1);
    pico_http_client_close(conn);
    /*Case6: positive test, no resource*/
    write_success_cnt = 0;
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post(conn, NULL, post_data, post_data_len, HTTP_CONN_CLOSE, NULL, NULL);
    fail_if(ret != HTTP_RETURN_OK);
    fail_if(write_success_cnt != 1);
    pico_http_client_close(conn);
    printf("Stop: tc_pico_http_client_send_post\n");
}
END_TEST
START_TEST(tc_pico_http_client_send_post_multipart)
{
    //TODO: test this: int8_t pico_http_client_send_post_multipart(uint16_t conn, char *resource, struct multipart_chunk **post_data, uint16_t post_data_len, uint8_t connection);;
    int32_t ret = 0;
    int16_t conn = 0;
    uint32_t post_data_len = 2;
    struct multipart_chunk **chunks1;
    unsigned char *data1 = (unsigned char*)"data1";
    unsigned char *data2 = (unsigned char*)"data2";
    char uri[50] = "http://user:pwd@httpbin.org/";
    printf("\n\nStart: tc_pico_http_client_send_post_multipart\n");
    chunks1 = PICO_ZALLOC(2 * sizeof(struct multipart_chunk *));
    chunks1[0] = multipart_chunk_create(data1, 5, "name1", "filename1", "attachment", NULL);
    chunks1[1] = multipart_chunk_create(data2, 5, "name2", "filename2", "attachment", NULL);
    /*Case1: unknown client*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post_multipart(99, "/", chunks1, post_data_len, HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    /*Case2: empty chunks (NULL)*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post_multipart(conn, "/", NULL, post_data_len, HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    /*Case3: unknown connection*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post_multipart(conn, "/", chunks1, post_data_len, 100);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    /*Case4: data_len = 0*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post_multipart(conn, "/", chunks1, 0, HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    /*Case5: positive test, everything got written at once*/
    write_success_cnt = 0;
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post_multipart(conn, "/", chunks1, post_data_len, HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_OK);
    fail_if(write_success_cnt != 1);
    pico_http_client_close(conn);
    /*Case6: positive test*/
    write_success_cnt = 0;
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post_multipart(conn, "/", chunks1, post_data_len, HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_OK);
    fail_if(write_success_cnt != 1);
    pico_http_client_close(conn);
    /*Case7: positive test, everything got written at once, no resource*/
    write_success_cnt = 0;
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post_multipart(conn, NULL, chunks1, post_data_len, HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_OK);
    fail_if(write_success_cnt != 1);
    pico_http_client_close(conn);
    multipart_chunk_destroy(chunks1[0]);
    multipart_chunk_destroy(chunks1[1]);
    PICO_FREE(chunks1);
    printf("Stop: tc_pico_http_client_send_post_multipart\n");
}
END_TEST
START_TEST(tc_pico_http_client_send_delete)
{
    //TODO: test this: int8_t pico_http_client_send_delete(uint16_t conn, char *resource, uint8_t connection);
    int32_t ret = 0;
    int16_t conn = 0;
    char uri[50] = "http://user:pwd@httpbin.org/";
    printf("\n\nStart: tc_pico_http_client_send_delete\n");
    /*Case1: unknown client*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_delete(99, "/", HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    /*Case2: unknown connection*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_delete(conn, "/", 100);
    fail_if(ret != HTTP_RETURN_ERROR);
    pico_http_client_close(conn);
    /*Case3: positive test, everything got written at once.*/
    write_success_cnt = 0;
    conn = pico_http_client_open(uri, cb);
    printf("Conn: %d\n", conn);
    ret = pico_http_client_send_delete(conn, "/", HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_OK);
    fail_if(write_success_cnt != 1);
    pico_http_client_close(conn);
    /*Case4: positive test, everything got written at once, no resource*/
    write_success_cnt = 0;
    conn = pico_http_client_open(uri, cb);
    printf("Conn: %d\n", conn);
    ret = pico_http_client_send_delete(conn, NULL, HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_OK);
    fail_if(write_success_cnt != 1);
    pico_http_client_close(conn);
    printf("Stop: tc_pico_http_client_send_delete\n");
}
END_TEST
START_TEST(tc_pico_http_client_close)
{
    //TODO: test this: int8_t pico_http_client_close(uint16_t conn);
    int32_t ret = 0;
    int16_t conn = 0;
    printf("\n\nStart: tc_pico_http_client_close\n");
    conn = pico_http_client_open("http://user:pwd@httpbin.org/", cb);
    /*Case1: unknown client*/
    ret = pico_http_client_close(99);
    fail_if(ret != HTTP_RETURN_ERROR);
    /*Case2: positive test*/
    ret = pico_http_client_close(conn);
    fail_if(ret != HTTP_RETURN_OK);
    printf("Stop: tc_pico_http_client_close\n");
}
END_TEST
START_TEST(tc_pico_http_client_long_poll_send_get)
{
    //TODO: test this: int8_t pico_http_client_long_poll_send_get(uint16_t conn, char *resource, uint8_t connection);
    int32_t ret = 0;
    int16_t conn = 0;
    printf("\n\nStart: tc_pico_http_client_long_poll_send_get\n");
    /*Case1: unknown client*/
    ret = pico_http_client_long_poll_send_get(99, "/", HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_ERROR);
    /*Case2: positive test*/
    conn = pico_http_client_open("http://user:pwd@httpbin.org/", cb);
    ret = pico_http_client_long_poll_send_get(conn, "/", HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_OK);
    ret = pico_http_client_long_poll_cancel(conn);
    /*Case3: positive test, no resource*/
    conn = pico_http_client_open("http://user:pwd@httpbin.org/", cb);
    ret = pico_http_client_long_poll_send_get(conn, NULL, HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_OK);
    ret = pico_http_client_long_poll_cancel(conn);
    printf("Stop: tc_pico_http_client_long_poll_send_get\n");
}
END_TEST
START_TEST(tc_pico_http_client_long_poll_cancel)
{
    //TODO: test this: int8_t pico_http_client_long_poll_cancel(uint16_t conn);
    int32_t ret = 0;
    int16_t conn = 0;
    printf("\n\nStart: tc_pico_http_client_long_poll_cancel\n");
    /*Case1: unknown client*/
    ret = pico_http_client_long_poll_cancel(99);
    fail_if(ret != HTTP_RETURN_ERROR);
    /*Case2: positive test*/
    conn = pico_http_client_open("http://user:pwd@httpbin.org/", cb);
    ret = pico_http_client_long_poll_send_get(conn, "/", HTTP_CONN_CLOSE);
    ret = pico_http_client_long_poll_cancel(conn);
    fail_if(ret != HTTP_RETURN_OK);
    printf("Stop: tc_pico_http_client_long_poll_cancel\n");
}
END_TEST
START_TEST(tc_pico_http_client_get_write_progress)
{
    //TODO: test this: int32_t pico_http_client_get_write_progress(uint16_t conn, uint32_t *total_bytes_written, uint32_t *total_bytes_to_write);
    int32_t ret = 0;
    int16_t conn = 0;
    unsigned char *post_data = (unsigned char*)"key=1&robbin=robbin";
    uint32_t post_data_len = strlen((char *)post_data);
    //char content_type[50] = "";
    //char cache_controle[50] = "";
    char uri[50] = "http://user:pwd@httpbin.org/";
    uint32_t total_bytes_written = 0;
    uint32_t total_bytes_to_write = 0;
    printf("\n\nStart: tc_pico_http_client_get_write_progress\n");
    /*Case1: positieve test (bytes returned in total_bytes_written,total_bytes_to_write | returns -1 when there is nothing to write)*/
    write_success_cnt = 0;
    write_in_chunks = 1;
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_post(conn, "/", post_data, post_data_len, HTTP_CONN_CLOSE, NULL, NULL);
    fail_if(ret != HTTP_RETURN_OK);
    ret = pico_http_client_get_write_progress(conn, &total_bytes_written, &total_bytes_to_write);
    printf("total_bytes_written: %d, total_bytes_to_write: %d\n", total_bytes_written, total_bytes_to_write);
    fail_if(ret != HTTP_RETURN_OK);
    treat_write_event(example_client);
    ret = pico_http_client_get_write_progress(conn, &total_bytes_written, &total_bytes_to_write);
    fail_if(ret != HTTP_RETURN_ERROR);
    fail_if(total_bytes_written < 0);
    fail_if(total_bytes_to_write < 0);
    fail_if(write_progress_made_cnt != 1);
    fail_if(write_success_cnt != 1);
    pico_http_client_close(conn);
    //Case2: No address for total_bytes_written
    write_in_chunks = 1;
    conn = pico_http_client_open(uri, cb);
    pico_http_client_send_post(conn, "/", post_data, post_data_len, HTTP_CONN_CLOSE, NULL, NULL);
    ret = pico_http_client_get_write_progress(conn, NULL, &total_bytes_to_write);
    fail_if(ret != HTTP_RETURN_ERROR);
    treat_write_event(example_client);
    pico_http_client_close(conn);
    //Case3: No address for total_bytes_to_write
    write_in_chunks = 1;
    conn = pico_http_client_open(uri, cb);
    pico_http_client_send_post(conn, "/", post_data, post_data_len, HTTP_CONN_CLOSE, NULL, NULL);
    ret = pico_http_client_get_write_progress(conn, &total_bytes_written, NULL);
    fail_if(ret != HTTP_RETURN_OK);
    treat_write_event(example_client);
    pico_http_client_close(conn);
    //Case4: Unknown connectionID
    write_in_chunks = 1;
    conn = pico_http_client_open(uri, cb);
    pico_http_client_send_post(conn, "/", post_data, post_data_len, HTTP_CONN_CLOSE, NULL, NULL);
    ret = pico_http_client_get_write_progress(99, &total_bytes_written, NULL);
    fail_if(ret != HTTP_RETURN_ERROR);
    treat_write_event(example_client);
    pico_http_client_close(conn);
    printf("Stop: tc_pico_http_client_get_write_progress\n");
}
END_TEST
START_TEST(tc_pico_http_client_read_header)
{
    /*TODO: test this: struct pico_http_header *pico_http_client_read_header(uint16_t conn);*/
    int ret = 0;
    int conn = 0;
    struct pico_http_header *header = NULL;
    char uri[50] = "http://user:pwd@httpbin.org/";
    header_ev_cnt = 0;
    read_header_in_chunks = 0;
    clear_read_idx = 1;
    printf("\n\nStart: tc_pico_http_client_read_header\n");
    /*Cse1: Positive case*/
    conn = pico_http_client_open(uri, cb);
    fail_if(conn < 0);
    ret = pico_http_client_send_get(conn, "/", HTTP_CONN_CLOSE);
    fail_if(ret != HTTP_RETURN_OK);
    fail_if(write_success_cnt != 1);
    treat_read_event(example_client);  //first time 10bytes will come in
    treat_read_event(example_client);  //second time de rest is available for reading.
    printf("header_ev_cnt, %d\n", header_ev_cnt); //fails here because lib can't handle the response if it comes in chunks
    fail_if(header_ev_cnt != 1);
    header = pico_http_client_read_header(conn);
    printf("Received header from server...\n");
    printf("Server response : %d\n",header->response_code);
    printf("Location : %s\n",header->location);
    printf("Transfer-Encoding : %d\n",header->transfer_coding);
    fail_if(header == NULL);
    /*Case2: Unknown connectionID*/
    header = pico_http_client_read_header(99);
    fail_if(header != NULL);
    pico_http_client_close(conn);
    printf("Stop: tc_pico_http_client_read_header\n");
}
END_TEST
START_TEST(tc_pico_http_client_read_uri_data)
{
    /*TODO: test this: struct pico_http_uri *pico_http_client_read_uri_data(uint16_t conn);*/
    int ret = 0;
    int conn = 0;
    struct pico_http_uri *urikey = NULL;
    char uri[50] = "http://user:pwd@httpbin.org/";
    clear_read_idx = 1;
    printf("\n\nStart: tc_pico_http_client_read_uri_data\n");
    /*Cse1: Positive case*/
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_get(conn, "/", HTTP_CONN_CLOSE);
    urikey = pico_http_client_read_uri_data(conn);
    fail_if(ret != HTTP_RETURN_OK);
    fail_if(urikey == NULL);
    /*Case2: Unknown connectionID*/
    urikey = pico_http_client_read_uri_data(99);
    fail_if(urikey != NULL);
    pico_http_client_close(conn);
    printf("Stop: tc_pico_http_client_read_uri_data\n");
}
END_TEST

START_TEST(tc_pico_http_client_read_body)
{
    /*TODO: test this: int32_t pico_http_client_read_body(uint16_t conn, uint8_t *data, uint16_t size, uint8_t *body_read_done);*/
    int ret = 0;
    int conn = 0;
    char uri[50] = "http://user:pwd@httpbin.org/";
    uint8_t body_read_done = 0;
    uint8_t *data = PICO_ZALLOC(1024*1024);
    struct pico_http_header *header = NULL;
    /*Case1: not chunked*/
    clear_read_idx = 1;
    printf("\n\nStart: tc_pico_http_client_read_body\n");
    //first the header needs to come in
    write_success_cnt = 0;
    header_ev_cnt = 0;
    body_ev_cnt = 0;
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_get(conn, "/", HTTP_CONN_CLOSE);
    fail_if(write_success_cnt != 1);
    treat_read_event(example_client);
    fail_if(header_ev_cnt != 1);
    header = pico_http_client_read_header(conn);
    printf("Received header from server...\n");
    printf("Server response : %d\n",header->response_code);
    printf("Location : %s\n",header->location);
    printf("Transfer-Encoding : %d\n",header->transfer_coding);
    printf("Size/Chunk : %d\n",header->content_length_or_chunk);
    fail_if(body_ev_cnt != 1);
    //then we can read the body
    ret = pico_http_client_read_body(conn, data, 1024, &body_read_done);
    write(0, data, ret);
    printf("body_read_done: %d ret: %d\n", body_read_done, ret);
    fail_if(ret != 36);
    fail_if(body_read_done != 1);
    pico_http_client_close(conn);
    PICO_FREE(data);
    printf("Stop: tc_pico_http_client_read_body\n");

    /*Case2: data in response is chunked*/
    clear_read_idx = 1;
    //first the header needs to come in
    write_success_cnt = 0;
    header_ev_cnt = 0;
    body_ev_cnt = 0;
    chunked_response = 1;
    body_read_done = 0;
    data = PICO_ZALLOC(1024*1024);
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_get(conn, "/", HTTP_CONN_CLOSE);
    fail_if(write_success_cnt != 1);
    treat_read_event(example_client);
    fail_if(header_ev_cnt != 1);
    header = pico_http_client_read_header(conn);
    printf("Received header from server...\n");
    printf("Server response : %d\n",header->response_code);
    printf("Location : %s\n",header->location);
    printf("Transfer-Encoding : %d\n",header->transfer_coding);
    printf("Size/Chunk : %d\n",header->content_length_or_chunk);
    fail_if(body_ev_cnt != 1);
    //then we can read the body
    int number_of_expected_chunks = 10;
    while (number_of_expected_chunks)
    {
        ret = pico_http_client_read_body(conn, data, 1024, &body_read_done);
        write(0, data, ret);
        printf("body_read_done: %d ret: %d\n", body_read_done, ret);
        number_of_expected_chunks--;
    }
    fail_if(body_read_done != 1);
    pico_http_client_close(conn);
    PICO_FREE(data);
    chunked_response = 0;
    printf("Stop: tc_pico_http_client_read_body\n");
    /*Case3: not chunked*/
    test_404_with_body = 1;
    clear_read_idx = 1;
    printf("\n\nStart: tc_pico_http_client_read_body\n");
    //first the header needs to come in
    write_success_cnt = 0;
    header_ev_cnt = 0;
    body_ev_cnt = 0;
    data = PICO_ZALLOC(1024*1024);
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_get(conn, "/", HTTP_CONN_CLOSE);
    fail_if(write_success_cnt != 1);
    treat_read_event(example_client);
    fail_if(header_ev_cnt != 1);
    header = pico_http_client_read_header(conn);
    printf("Received header from server...\n");
    printf("Server response : %d\n",header->response_code);
    printf("Location : %s\n",header->location);
    printf("Transfer-Encoding : %d\n",header->transfer_coding);
    printf("Size/Chunk : %d\n",header->content_length_or_chunk);
    //fail_if(body_ev_cnt != 1);
    //then we can read the body
    ret = pico_http_client_read_body(conn, data, 1024, &body_read_done);
    write(0, data, ret);
    printf("body_read_done: %d ret: %d\n", body_read_done, ret);
    fail_if(ret != 233);
    fail_if(body_read_done != 1);
    pico_http_client_close(conn);
    PICO_FREE(data);
    printf("Stop: tc_pico_http_client_read_body\n");
    chunked_response = 0;
    test_404_with_body = 0;

    /*Case4: not chunked
    test_404_without_body = 1;
    clear_read_idx = 1;
    printf("\n\nStart: tc_pico_http_client_read_body\n");
    //first the header needs to come in
    write_success_cnt = 0;
    header_ev_cnt = 0;
    body_ev_cnt = 0;
    data = PICO_ZALLOC(1024*1024);
    conn = pico_http_client_open(uri, cb);
    ret = pico_http_client_send_get(conn, "/", HTTP_CONN_CLOSE);
    fail_if(write_success_cnt != 1);
    treat_read_event(example_client);
    fail_if(header_ev_cnt != 1);
    header = pico_http_client_read_header(conn);
    printf("Received header from server...\n");
    printf("Server response : %d\n",header->response_code);
    printf("Location : %s\n",header->location);
    printf("Transfer-Encoding : %d\n",header->transfer_coding);
    printf("Size/Chunk : %d\n",header->content_length_or_chunk);
    //fail_if(body_ev_cnt != 1);
    //then we can read the body
    ret = pico_http_client_read_body(conn, data, 1024, &body_read_done);
    write(0, data, ret);
    printf("body_read_done: %d ret: %d\n", body_read_done, ret);
    fail_if(ret != 233);
    fail_if(body_read_done != 1);
    pico_http_client_close(conn);
    PICO_FREE(data);
    printf("Stop: tc_pico_http_client_read_body\n");
    chunked_response = 0;
    test_404_without_body = 0;*/
}
END_TEST

/* API end */

/*
START_TEST(tc_free_uri)
{
   // TODO: test this: static int8_t free_uri(struct pico_http_client *to_be_removed);
    int ret = 0;
    printf("\n\nStart: tc_free_uri\n");
    ret = free_uri(NULL);
    fail_if(ret != HTTP_RETURN_ERROR);

    struct pico_http_client *client = NULL;
    client = PICO_ZALLOC(sizeof(struct pico_http_client));
    client->urikey = PICO_ZALLOC(sizeof(struct pico_http_uri));
    ret = free_uri(client);
    fail_if(ret != HTTP_RETURN_OK);
    printf("Stop: tc_free_uri\n");
}
END_TEST
START_TEST(tc_client_open)
{
   // TODO: test this: static int32_t client_open(char *uri, void (*wakeup)(uint16_t ev, uint16_t conn), int32_t connID);
}
END_TEST
START_TEST(tc_free_header)
{
   // TODO: test this: static void free_header(struct pico_http_client *to_be_removed);
}
END_TEST
START_TEST(tc_print_request_part_info)
{
   // TODO: test this: static void print_request_part_info(struct pico_http_client *client)
}
END_TEST
START_TEST(tc_request_parts_destroy)
{
   // TODO: test this: static int8_t request_parts_destroy(struct pico_http_client *client)
    int ret = 0;
    ret = request_parts_destroy(NULL);
    fail_if(ret != HTTP_RETURN_ERROR);
}
END_TEST
START_TEST(tc_socket_write_request_parts)
{
   // TODO: test this: static int32_t socket_write_request_parts(struct pico_http_client *client)
}
END_TEST

START_TEST(tc_compare_clients)
{
   // TODO: test this: static int32_t compare_clients(void *ka, void *kb)
}
END_TEST
START_TEST(tc_parse_header_from_server)
{
   // TODO: test this: static int8_t parse_header_from_server(struct pico_http_client *client, struct pico_http_header *header);
}
END_TEST
START_TEST(tc_read_chunk_line)
{
   // TODO: test this: static int8_t read_chunk_line(struct pico_http_client *client);
}
END_TEST
START_TEST(tc_wait_for_header)
{
   // TODO: test this: static inline void wait_for_header(struct pico_http_client *client) 
}
END_TEST
START_TEST(tc_treat_write_event)
{
   // TODO: test this: static void treat_write_event(struct pico_http_client *client)
}
END_TEST
START_TEST(tc_treat_read_event)
{
   // TODO: test this: static void treat_read_event(struct pico_http_client *client)
}
END_TEST
START_TEST(tc_treat_long_polling)
{
   // TODO: test this: static void treat_long_polling(struct pico_http_client *client, uint16_t ev)
}
END_TEST
START_TEST(tc_tcp_callback)
{
   // TODO: test this: static void tcp_callback(uint16_t ev, struct pico_socket *s)
}
END_TEST
START_TEST(tc_dns_callback)
{
   // TODO: test this: static void dns_callback(uint8_t *ip, void *ptr)
}
END_TEST
START_TEST(tc_pico_http_client_build_delete)
{
   // TODO: test this: static int8_t *pico_http_client_build_delete(const struct pico_http_uri *uri_data, uint8_t connection)
}
END_TEST
START_TEST(tc_get_content_length)
{
   // TODO: test this: static int32_t get_content_length(struct multipart_chunk **post_data, uint16_t length, uint8_t *boundary)
}
END_TEST
START_TEST(tc_get_max_multipart_header_size)
{
   // TODO: test this: static int32_t get_max_multipart_header_size(struct multipart_chunk **post_data, uint16_t length)
}
END_TEST
START_TEST(tc_add_multipart_chunks)
{
   // TODO: test this: static int8_t add_multipart_chunks(struct multipart_chunk **post_data, uint16_t post_data_length, uint8_t *boundary, struct pico_http_client *http)
}
END_TEST
START_TEST(tc_pico_http_client_build_post_multipart_request)
{
   // TODO: test this: static int8_t pico_http_client_build_post_multipart_request(const struct pico_http_uri *uri_data, struct multipart_chunk **post_data, uint16_t len, struct pico_http_client *http, uint8_t connection) 
}
END_TEST
START_TEST(tc_pico_http_client_build_post_header)
{
   // TODO: test this: static int8_t *pico_http_client_build_post_header(const struct pico_http_uri *uri_data, uint32_t post_data_len, uint8_t connection, uint8_t *content_type, uint8_t *cache_control)
}
END_TEST
START_TEST(tc_check_chunk_line)
{
   // TODO: test this: static inline int check_chunk_line(struct pico_http_client *client, int tmp_len_read)
}
END_TEST
START_TEST(tc_update_content_length)
{
   // TODO: test this: static inline void update_content_length(struct pico_http_client *client, uint32_t tmp_len_read )
}
END_TEST
START_TEST(tc_read_body)
{
   // TODO: test this: static inline int32_t read_body(struct pico_http_client *client, uint8_t *data, uint16_t size, uint32_t *len_read, uint32_t *tmp_len_read)
}
END_TEST
START_TEST(tc_read_big_chunk)
{
   // TODO: test this: static inline uint32_t read_big_chunk(struct pico_http_client *client, uint8_t *data, uint16_t size, uint32_t *len_read)
}
END_TEST
START_TEST(tc_read_small_chunk)
{
   // TODO: test this: static inline void read_small_chunk(struct pico_http_client *client, uint8_t *data, uint16_t size, uint32_t *len_read)
}
END_TEST
START_TEST(tc_read_chunked_data)
{
   // TODO: test this: static inline int32_t read_chunked_data(struct pico_http_client *client, uint8_t *data, uint16_t size)
}
END_TEST
START_TEST(tc_read_first_line)
{
   // TODO: test this: static inline void read_first_line(struct pico_http_client *client, uint8_t *line, uint32_t *index)
}
END_TEST
START_TEST(tc_start_reading_body)
{
   // TODO: test this: static inline void start_reading_body(struct pico_http_client *client, struct pico_http_header *header)
}
END_TEST
START_TEST(tc_parse_loc_and_cont)
{
   // TODO: test this: static inline int32_t parse_loc_and_cont(struct pico_http_client *client, struct pico_http_header *header, uint8_t *line, uint32_t *index)
}
END_TEST
START_TEST(tc_parse_transfer_encoding)
{
   // TODO: test this: static inline int32_t parse_transfer_encoding(struct pico_http_client *client, struct pico_http_header *header, uint8_t *line, uint32_t *index)
}
END_TEST
START_TEST(tc_parse_fields)
{
   // TODO: test this: static inline int32_t parse_fields(struct pico_http_client *client, struct pico_http_header *header, int8_t *line, uint32_t *index)
}
END_TEST
START_TEST(tc_parse_rest_of_header)
{
   // TODO: test this: static inline int32_t parse_rest_of_header(struct pico_http_client *client, struct pico_http_header *header, uint8_t *line, uint32_t *index)
}
END_TEST
START_TEST(tc_set_client_chunk_state)
{
   // TODO: test this: static inline void set_client_chunk_state(struct pico_http_client *client)
}
END_TEST
START_TEST(tc_read_chunk_trail)
{
   // TODO: test this: static inline void read_chunk_trail(struct pico_http_client *client)
}
END_TEST
START_TEST(tc_read_chunk_value)
{
   // TODO: test this: static inline void read_chunk_value(struct pico_http_client *client)
}
END_TEST
*/

Suite *pico_suite(void)
{
    Suite *s = suite_create("PicoTCP-modules HTTPLIB");

    /*API start*/
    TCase *TCase_multipart_chunk_create = tcase_create("Unit test for tc_multipart_chunk_create");
    TCase *TCase_multipart_chunk_destroy = tcase_create("Unit test for tc_multipart_chunk_destroy");
    TCase *TCase_pico_process_uri = tcase_create("Unit test for tc_pico_process_uri");
    TCase *TCase_tc_base64_encode = tcase_create("Unit test for Base64 Encoding");
    TCase *TCase_pico_http_client_open = tcase_create("Unit test for tc_pico_http_client_open");
    TCase *TCase_pico_http_client_send_raw = tcase_create("Unit test for tc_pico_http_client_send_raw");
    TCase *TCase_pico_http_client_send_get = tcase_create("Unit test for tc_pico_http_client_send_get");
    TCase *TCase_pico_http_client_send_post = tcase_create("Unit test for tc_pico_http_client_send_post");
    TCase *TCase_pico_http_client_send_post_multipart = tcase_create("Unit test for tc_pico_http_client_send_post_multipart");
    TCase *TCase_pico_http_client_send_delete = tcase_create("Unit test for tc_pico_http_client_send_delete");
    TCase *TCase_pico_http_client_close = tcase_create("Unit test for tc_pico_http_client_close");
    TCase *TCase_pico_http_client_long_poll_send_get = tcase_create("Unit test for tc_pico_http_client_long_poll_send_get");
    TCase *TCase_pico_http_client_long_poll_cancel = tcase_create("Unit test for tc_pico_http_client_long_poll_cancel");
    TCase *TCase_pico_http_client_get_write_progress = tcase_create("Unit test for tc_pico_http_client_get_write_progress");
    TCase *TCase_pico_http_client_read_header = tcase_create("Unit test for tc_pico_http_client_read_header");
    TCase *TCase_pico_http_client_read_uri_data = tcase_create("Unit test for tc_pico_http_client_read_uri_data");
    TCase *TCase_pico_http_client_read_body = tcase_create("Unit test for tc_pico_http_client_read_body");

    /*API end*/


    /*TCase *TCase_free_uri = tcase_create("Unit test for free_uri");
    TCase *TCase_client_open = tcase_create("Unit test for client_open");
    TCase *TCase_free_header = tcase_create("Unit test for free_header");
    TCase *TCase_print_request_part_info = tcase_create("Unit test for print_request_part_info");
    TCase *TCase_request_parts_destroy = tcase_create("Unit test for request_parts_destroy");
    TCase *TCase_socket_write_request_parts = tcase_create("Unit test for socket_write_request_parts");
    TCase *TCase_compare_clients = tcase_create("Unit test for compare_clients");
    TCase *TCase_parse_header_from_server = tcase_create("Unit test for parse_header_from_server");
    TCase *TCase_read_chunk_line = tcase_create("Unit test for read_chunk_line");
    TCase *TCase_wait_for_header = tcase_create("Unit test for wait_for_header");
    TCase *TCase_treat_write_event = tcase_create("Unit test for treat_write_event");
    TCase *TCase_treat_read_event = tcase_create("Unit test for treat_read_event");
    TCase *TCase_treat_long_polling = tcase_create("Unit test for treat_long_polling");
    TCase *TCase_tcp_callback = tcase_create("Unit test for tcp_callback");
    TCase *TCase_dns_callback = tcase_create("Unit test for dns_callback");
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
    */

    /*API start*/
    tcase_add_test(TCase_multipart_chunk_create, tc_multipart_chunk_create);
    suite_add_tcase(s, TCase_multipart_chunk_create);
    tcase_add_test(TCase_multipart_chunk_destroy, tc_multipart_chunk_destroy);
    suite_add_tcase(s, TCase_multipart_chunk_destroy);
    tcase_add_test(TCase_pico_process_uri, tc_pico_process_uri);
    suite_add_tcase(s, TCase_pico_process_uri);
    tcase_add_test(TCase_tc_base64_encode, tc_base64_encode);
    suite_add_tcase(s, TCase_tc_base64_encode);
    tcase_add_test(TCase_pico_http_client_open, tc_pico_http_client_open);
    suite_add_tcase(s, TCase_pico_http_client_open);
    tcase_add_test(TCase_pico_http_client_send_raw, tc_pico_http_client_send_raw);
    suite_add_tcase(s, TCase_pico_http_client_send_raw);
    tcase_add_test(TCase_pico_http_client_send_get, tc_pico_http_client_send_get);
    suite_add_tcase(s, TCase_pico_http_client_send_get);
    tcase_add_test(TCase_pico_http_client_send_post, tc_pico_http_client_send_post);
    suite_add_tcase(s, TCase_pico_http_client_send_post);
    tcase_add_test(TCase_pico_http_client_send_post_multipart, tc_pico_http_client_send_post_multipart);
    suite_add_tcase(s, TCase_pico_http_client_send_post_multipart);
    tcase_add_test(TCase_pico_http_client_send_delete, tc_pico_http_client_send_delete);
    suite_add_tcase(s, TCase_pico_http_client_send_delete);
    tcase_add_test(TCase_pico_http_client_close, tc_pico_http_client_close);
    suite_add_tcase(s, TCase_pico_http_client_close);
    tcase_add_test(TCase_pico_http_client_long_poll_send_get, tc_pico_http_client_long_poll_send_get);
    suite_add_tcase(s, TCase_pico_http_client_long_poll_send_get);
    tcase_add_test(TCase_pico_http_client_long_poll_cancel, tc_pico_http_client_long_poll_cancel);
    suite_add_tcase(s, TCase_pico_http_client_long_poll_cancel);
    tcase_add_test(TCase_pico_http_client_get_write_progress, tc_pico_http_client_get_write_progress);
    suite_add_tcase(s, TCase_pico_http_client_get_write_progress);
    tcase_add_test(TCase_pico_http_client_read_header, tc_pico_http_client_read_header);
    suite_add_tcase(s, TCase_pico_http_client_read_header);
    tcase_add_test(TCase_pico_http_client_read_uri_data, tc_pico_http_client_read_uri_data);
    suite_add_tcase(s, TCase_pico_http_client_read_uri_data);
    tcase_add_test(TCase_pico_http_client_read_body, tc_pico_http_client_read_body);
    suite_add_tcase(s, TCase_pico_http_client_read_body);
    /*API end*/


    /*tcase_add_test(TCase_free_uri, tc_free_uri);
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
    */
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
