#include <Arduino.h>
#include <lwip/sockets.h>
#include <string.h>
#include "RoboCarListen.h"

int roboListen(uint16_t port)
{
    int sock = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) return -1;
    int reuse = 1;
    lwip_setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));    // sin_addr 0 = INADDR_ANY
    addr.sin_family = AF_INET;
    addr.sin_port = PP_HTONS(port);
    int nonBlocking = 1;
    if (lwip_bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0 || lwip_listen(sock, 2) < 0 ||
        lwip_ioctl(sock, FIONBIO, &nonBlocking) < 0) {
        lwip_close(sock);
        return -1;
    }
    return sock;
}

int roboAccept(int listener, int recvTimeoutMs)
{
    if (listener < 0) return -1;
    int sock = lwip_accept(listener, NULL, NULL);
    if (sock < 0) return -1;
    // WiFiClient(uint8_t) stores the socket in a uint8_t; 0xFF means "no socket".
    if (sock >= 0xFF) { lwip_close(sock); return -1; }
    // Match the SDK's accepted-socket setup: blocking I/O with bounded timeouts.
    int blocking = 0, enable = 1, sendTimeout = 10000;
    lwip_ioctl(sock, FIONBIO, &blocking);
    lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &recvTimeoutMs, sizeof(recvTimeoutMs));
    lwip_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &sendTimeout, sizeof(sendTimeout));
    lwip_setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
    return sock;
}

// Zero-timeout select(): lets loop() skip idle browser "preconnect" sockets
// instead of blocking in a read until the socket receive timeout expires.
int roboReadable(int sock)
{
    if (sock < 0) return -1;
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    int n = lwip_select(sock + 1, &readSet, NULL, NULL, &tv);
    if (n < 0) return -1;
    return (n > 0 && FD_ISSET(sock, &readSet)) ? 1 : 0;
}

int roboConnect(uint32_t ip, uint16_t port, int recvTimeoutMs, int sendTimeoutMs)
{
    int sock = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) return -1;
    if (sock >= 0xFF) { lwip_close(sock); return -1; }    // must fit WiFiClient(uint8_t)
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;    // IPAddress stores the address in network byte order
    addr.sin_port = PP_HTONS(port);
    if (lwip_connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        lwip_close(sock);
        return -1;
    }
    int enable = 1;
    lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &recvTimeoutMs, sizeof(recvTimeoutMs));
    lwip_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &sendTimeoutMs, sizeof(sendTimeoutMs));
    lwip_setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
    return sock;
}

// Blocking send bounded by the socket's SO_SNDTIMEO.
bool roboSend(int sock, const uint8_t *data, size_t length)
{
    if (sock < 0) return false;
    while (length > 0) {
        size_t chunk = length > 1460 ? 1460 : length;
        int n = lwip_send(sock, data, chunk, 0);
        if (n <= 0) return false;    // error or send timeout
        data += n;
        length -= n;
    }
    return true;
}

void roboCloseSocket(int sock)
{
    if (sock >= 0) lwip_close(sock);
}
