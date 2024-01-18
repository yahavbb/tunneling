#include <sys/socket.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

#include "vpn.h"

#define BUFFERSIZE 1500
#define PORT 4555

SSL_CTX *init_ssl()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx = NULL;
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    method = TLS_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        return ctx;
    }
#ifdef CLIENT
    if (1 != SSL_CTX_load_verify_locations(ctx, CA_CERT_FILE, NULL))
    {
        exit(EXIT_FAILURE);
    }
#endif /* CLIENT */

    if (1 != SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM))
    {
        exit(EXIT_FAILURE);
    }

    if (1 != SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM))
    {
        exit(EXIT_FAILURE);
    }

    if (1 != SSL_CTX_check_private_key(ctx))
    {
        exit(EXIT_FAILURE);
    }
    return ctx;
}

static int TcpServer(int socket_desc, struct sockaddr_in client_addr, SSL_CTX **ctx, SSL **ssl)
{
    int client_sock = 0;
    socklen_t client_size = sizeof(struct sockaddr_in);

    if (0 > bind(socket_desc, (struct sockaddr *)&client_addr, client_size))
    {
        close(socket_desc);
        return -1;
    }
    if (0 > listen(socket_desc, 1))
    {
        return -1;
    }
    client_sock = accept(socket_desc, (struct sockaddr *)&client_addr, &client_size);
    if (0 > client_sock)
    {
        return -1;
    }

    *ssl = SSL_new(*ctx);
    SSL_set_fd(*ssl, socket_desc);

    if (!SSL_accept(*ssl))
    {
        printf("SSL connection failed\n");
        return -1;
    }

    return client_sock;
}
static int TcpClient(int socket_desc, struct sockaddr_in server_addr, SSL_CTX **ctx, SSL **ssl)
{
    if (0 > connect(socket_desc, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)))
    {
        return -1;
    }

    *ssl = SSL_new(*ctx);
    SSL_set_fd(*ssl, socket_desc);
    if (!SSL_connect(*ssl))
    {
        return -1;
    }
    return 0;
}
int TcpSslConnection(SSL_CTX **ctx, SSL **ssl)
{
    /* create socket */
    struct sockaddr_in addr;
    int sd = 0;

    sd = socket(AF_INET, SOCK_STREAM, 0);

    if (sd < 0)
    {
        return -1;
    }

    /* set port and ip */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);

#ifdef SERVER
    addr.sin_addr.s_addr = INADDR_ANY;

    if (0 > (TcpServer(sd, addr, ctx, ssl)))
    {
        return -1;
    }
#else
    addr.sin_addr.s_addr = inet_addr(IPSERVER);

    if (0 > (TcpClient(sd, addr, ctx, ssl)))
    {
        return -1;
    }

#endif
    return sd;
}
int TunTraffic(int vnic, SSL *ssl)
{
    int ret = 0;
    char buffer[BUFFERSIZE];

    ret = read(vnic, buffer, sizeof(buffer));
    if (0 > ret)
    {
        return -1;
    }

    ret = SSL_write(ssl, buffer, ret);
    if (0 >= ret)
    {
        return -1;
    }

    memset(buffer, 0, sizeof(buffer));
    return 0;
}

int TcpTraffic(int vnic, SSL *ssl)
{
    int ret = 0;
    char buffer[BUFFERSIZE];

    ret = SSL_read(ssl, buffer, sizeof(buffer));
    if (0 >= ret)
    {
        return -1;
    }

    ret = write(vnic, (const void *)buffer, ret);
    if (-1 == ret)
    {
        return -1;
    }
    memset(buffer, 0, sizeof(buffer));
    return 0;
}
