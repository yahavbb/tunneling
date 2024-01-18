#define _XOPEN_SOURCE 500 /*snprintf*/
#include <linux/if.h>
#include <linux/if_tun.h>
#include <signal.h>
#include <sys/ioctl.h> /*TUNSETIFF, ioctl*/
#include <string.h>    /*memset*/
#include <unistd.h>    /*close*/
#include <fcntl.h>     /*open*/
#include <stdio.h>     /*sprintf*/
#include <stdlib.h>    /*EXIT_SUCCESS*/
#include <stdio.h>     /*snprintf*/

#include "vpn.h"

#define BUFFER_SIZE 1024
#define MTU 1400
#define SERVER_HOST "192.168.4.14"

static int ExcCmd(char *cmd);
static void Cleanup(int signo);

int tun_alloc(char* dev)
{
	struct ifreq ifr;
	int fd = 0, err = 0;
	char* clonedev = "/dev/net/tun";
	
    memset(&ifr, 0, sizeof(ifr));

	if(0 > (fd = open(clonedev, O_RDWR)))
	{
		return fd;
	}


	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

	if(*dev)
	{
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	}

	if(0 > (err = ioctl(fd, TUNSETIFF, (void*) &ifr)))
	{
		close(fd);
		return err;
	}

	strcpy(dev, ifr.ifr_name);

	return fd;
}

int IfConfig()
{
    char cmd[BUFFER_SIZE] = {0};
    snprintf(cmd, sizeof(cmd), "ifconfig tun0 %s mtu %d up", IPTUN, MTU);

    return ExcCmd(cmd);
}
static int ExcCmd(char *cmd)
{
    if (system(cmd))
    {
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}

void SetupRoutlins()
{
#ifdef CLIENT
    char cmd[BUFFER_SIZE] = {0};
    ExcCmd("iptables -t nat -A POSTROUTING -o tun0 -j MASQUERADE");
    ExcCmd("iptables -I FORWARD 1 -i tun0 -m state --state RELATED,ESTABLISHED -j ACCEPT");
    ExcCmd("iptables -I FORWARD 1 -o tun0 -j ACCEPT");
    snprintf(cmd, sizeof(cmd), "ip route add %s via $(route -n | grep 'UG[ \t]' | awk '{print $2}')", IPSERVER);
    ExcCmd(cmd);
    ExcCmd("ip route add 0/1 dev tun0");
    ExcCmd("ip route add 128/1 dev tun0");
#else
    ExcCmd("iptables -t nat -A POSTROUTING -s 10.8.0.0/16 ! -d 10.8.0.0/16 -m comment --comment 'vpndemo' -j MASQUERADE");
    ExcCmd("iptables -A FORWARD -s 10.8.0.0/16 -m state --state RELATED,ESTABLISHED -j ACCEPT");
    ExcCmd("iptables -A FORWARD -d 10.8.0.0/16 -j ACCEPT");
#endif
    ExcCmd("sysctl -w net.ipv4.ip_forward=1");
}

void CleanupRoutlins()
{
#ifdef CLIENT
    char cmd[BUFFER_SIZE] = {0};
    ExcCmd("iptables -t nat -D POSTROUTING -o tun0 -j MASQUERADE");
    ExcCmd("iptables -D FORWARD -i tun0 -m state --state RELATED,ESTABLISHED -j ACCEPT");
    ExcCmd("iptables -D FORWARD -o tun0 -j ACCEPT");
    snprintf(cmd, sizeof(cmd), "ip route del %s", SERVER_HOST);
    ExcCmd(cmd);
    ExcCmd("ip route tun0 0/1");
    ExcCmd("ip route tun0 128/1");
#else
    ExcCmd("iptables -t nat -D POSTROUTING -s 10.8.0.0/16 ! -d 10.8.0.0/16 -m comment --comment 'vpndemo' -j MASQUERADE");
    ExcCmd("iptables -D FORWARD -s 10.8.0.0/16 -m state --state RELATED,ESTABLISHED -j ACCEPT");
    ExcCmd("iptables -D FORWARD -d 10.8.0.0/16 -j ACCEPT");
#endif
}

static void Cleanup(int signo)
{
    if (signo == SIGINT || signo == SIGTERM || signo == SIGHUP || signo)
    {
        CleanupRoutlins();
        exit(EXIT_SUCCESS);
    }
}
int CleanupSig()
{
    struct sigaction sa;
    sa.sa_handler = &Cleanup;

    if (0 > sigaction(SIGHUP, &sa, NULL))
    {
        return EXIT_FAILURE;
    }
    if (0 > sigaction(SIGINT, &sa, NULL))
    {
        return EXIT_FAILURE;
    }
    if (0 > sigaction(SIGTERM, &sa, NULL))
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
