#pragma once
#include <stddef.h>
#include <stdint.h>

// Raw lwIP TCP helpers: a non-blocking listener (same approach as ILI9341_Giraffe/PlushPetListen),
// an outgoing connect and a send that bypasses WiFiClient::write() (broken on the ideasHatch core).
// The SDK WiFiServer either blocks loop() in accept() or, in non-blocking mode, prints
// "[ERROR] get_available Accept connection failed" on every idle poll.
// Kept in its own .cpp because lwIP's socket macros (read/write/close)
// collide with WiFiClient methods.
int roboListen(uint16_t port);                    // listening socket, or -1
int roboAccept(int listener, int recvTimeoutMs);  // connected socket, or -1 if none is pending
int roboReadable(int sock);                       // 1 = data (or EOF) ready, 0 = nothing yet, -1 = error
int roboConnect(uint32_t ip, uint16_t port, int recvTimeoutMs, int sendTimeoutMs);  // ip as IPAddress -> uint32_t
bool roboSend(int sock, const uint8_t *data, size_t length);   // true when every byte was sent
void roboCloseSocket(int sock);
