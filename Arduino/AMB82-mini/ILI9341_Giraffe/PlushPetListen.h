#pragma once
#include <stdint.h>

// Non-blocking TCP listener on lwIP sockets. The SDK WiFiServer either blocks
// loop() in accept() or, in non-blocking mode, prints
// "[ERROR] get_available Accept connection failed" on every idle poll.
// Kept in its own .cpp because lwIP's socket macros (read/write/close)
// collide with WiFiClient methods.
int petListen(uint16_t port);   // listening socket, or -1
int petAccept(int listener);    // connected socket, or -1 if none is pending
void petCloseSocket(int sock);
