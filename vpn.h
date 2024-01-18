#ifndef __VPN_H__ILRD__CR3_
#define __VPN_H__ILRD__CR3_

#include <openssl/ssl.h> /*openssl*/

#define SERVER 1

int TcpSslConnection(SSL_CTX **ctx, SSL **ssl);
int TunTraffic(int vnic, SSL *ssl);
int TcpTraffic(int vnic, SSL *ssl);
int tun_alloc(char *dev);
SSL_CTX *init_ssl();
int IfConfig();
void SetupRoutlins();
void CleanupRoutlins();
int CleanupSig();

#ifdef SERVER
    #define IPTUN "10.8.0.1/16"
    #define CERT_FILE "/home/yahav/certs/server.crt"
    #define KEY_FILE "/home/yahav/certs/server.key"
#else
    #define IPTUN "10.8.0.2/16"
    #define IPSERVER "192.168.4.14"
    #define CLIENT 1
    #define CERT_FILE "/home/yahav/cert/client.crt"
    #define KEY_FILE "/home/yahav/cert/client.key"
    #define CA_CERT_FILE "/home/yahav/cert/myCA.pem"

#endif /* SERVER */

#endif /*__VPN_H__ILRD__CR3_*/