#include <stdlib.h>      /*EXIT_SUCCESS*/
#include <stdio.h>       /*printf*/
#include <sys/select.h>  /*select*/
#include <unistd.h>      /*close*/
#include <openssl/ssl.h> /*openssl */
#include "vpn.h"

#define PORT 4445
#define SINGLE 1
#define MTU 1400

#define SERVER 1

void Cleanup(int tun_fd, int tcp_fd, SSL_CTX *ctx, SSL *ssl)
{
    
    SSL_CTX_free(ctx);
    SSL_free(ssl);
    close(tun_fd);
    system("sudo ip link delete tun0");
    close(tcp_fd);
    CleanupRoutlins();
}

int max(int x, int y)
{
    return x > y ? x : y;
}

int main()
{
    fd_set rfds;
    char tunname[24] = "tun0";
    int tun_fd = tun_alloc(tunname);
    int tcp_fd = 0;
    int maxfdp = 0;
    SSL_CTX *ctx = init_ssl();
    SSL *ssl = NULL;

    if (0 > tun_fd)
    {
        return EXIT_FAILURE;
    }

    tcp_fd = TcpSslConnection(&ctx, &ssl);
    if (0 > tcp_fd)
    {
        close(tun_fd);
        return EXIT_FAILURE;
    }
    
    if (IfConfig())
    {
        Cleanup(tun_fd, tcp_fd, ctx, ssl);
        return EXIT_FAILURE;
    }

    SetupRoutlins();
    CleanupSig();

    while (1)
    {
        FD_ZERO(&rfds);

        FD_SET(tcp_fd, &rfds);
        FD_SET(tun_fd, &rfds);

        maxfdp = max(tcp_fd, tun_fd);

        if (0 > select(maxfdp + 1, &rfds, NULL, NULL, NULL))
        {
            return EXIT_FAILURE;
        }

        if (FD_ISSET(tcp_fd, &rfds))
        {
            if (-1 == TcpTraffic(tun_fd, ssl))
            {
                Cleanup(tun_fd, tcp_fd, ctx, ssl);
                return -1;
            }
        }
        if (FD_ISSET(tun_fd, &rfds))
        {
            if (-1 == TunTraffic(tun_fd, ssl))
            {
                Cleanup(tun_fd, tcp_fd, ctx, ssl);
                return -1;
            }
        }
    }
    return EXIT_SUCCESS;
}