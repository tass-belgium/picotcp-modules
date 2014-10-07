#include "pico_https_glue.h"

int pico_EmbedSend(CYASSL* ssl, char *buf, int sz, void *ctx)
{
    struct pico_socket *sock = (struct pico_socket *) ctx;
    int sent;
    int len = sz;

    sent = (int)pico_socket_write(sock, buf, len);
	if (sent != 0)
		return sent;
	else
		return -2;
}

int pico_EmbedReceive(CYASSL *ssl, char *buf, int sz, void *ctx)
{
	struct pico_socket *sock = (struct pico_socket *) ctx;
    int read;
    int len = sz; 

    read = (int)pico_socket_read(sock, buf, len);
	if (read != 0)
		return read;
	else
		return -2; // WANT_READ. Needed for non_blocking reads
}
