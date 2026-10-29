#include <Arduino.h>
#include <lwip/sockets.h>
#include <string.h>
#include "PlushPetListen.h"

int petListen(uint16_t port)
{
    int sock = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) return -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr)); // sin_addr 0 = INADDR_ANY
    addr.sin_family = AF_INET;
    addr.sin_port = PP_HTONS(port);
    int nonBlocking = 1;
    if (lwip_bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0 || lwip_listen(sock, 2) < 0 ||
        lwip_ioctl(sock, FIONBIO, &nonBlocking) < 0) {
        lwip_close(sock); return -1;
    }
    return sock;
}

int petAccept(int listener)
{
    if (listener < 0) return -1;
    int sock = lwip_accept(listener, NULL, NULL);
    if (sock < 0) return -1;
    // Match the SDK's accepted-socket setup: blocking I/O with bounded timeouts.
    int blocking = 0, enable = 1, recvTimeout = 3000, sendTimeout = 30000;
    lwip_ioctl(sock, FIONBIO, &blocking);
    lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));
    lwip_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &sendTimeout, sizeof(sendTimeout));
    lwip_setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
    return sock;
}

void petCloseSocket(int sock)
{
    if (sock >= 0) lwip_close(sock);
}
