#ifndef __VPN_
#define __VPN_

#include <openssl/ssl.h> /*openssl*/

#define SERVER 0

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
    #define IPTUN " "
    #define CERT_FILE "/server.crt"
    #define KEY_FILE "/server.key"
#else
    #define IPTUN "IP/16"
    #define IPSERVER " "
    #define CLIENT 1
    #define CERT_FILE "/client.crt"
    #define KEY_FILE "/client.key"
    #define CA_CERT_FILE "/myCA.pem"

#endif /* SERVER */

#endif /*__VPN_*/
