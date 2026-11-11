#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <stdlib.h>

// HTTP/1.x message reader shared by the Gemma-4 client and the web server
// (adapted from ILI9341_Giraffe/PlushPetHttp.h).
//
// The SDK's WiFiClient::available() spins forever on an idle blocking socket
// (it retries on EAGAIN), so it is never used here. read() returns after at most
// the socket receive timeout; one absolute deadline bounds the whole message.
// The optional idle() hook runs whenever a read times out without data; returning
// false from it cancels the transfer (used to keep the RoboCar responsive and
// abortable while Gemma-4 is thinking).
class RoboHttpReader {
public:
    typedef bool (*IdleHook)();

    WiFiClient &client;
    uint32_t started, timeout;
    IdleHook idle;
    int status = 0;
    long remaining = -1;
    bool chunked = false, failed = false, done = false, cancelled = false, timedOut = false;
    size_t chunkRemaining = 0;
    bool chunkCRLF = false;
    String contentType;
    String accept;    // request "Accept" header (browsers send text/html, curl sends */*)

    RoboHttpReader(WiFiClient &c, uint32_t t, IdleHook hook = nullptr)
        : client(c), started(millis()), timeout(t), idle(hook) {}

    int raw()
    {
        while (millis() - started < timeout) {
            uint8_t c;
            int n = client.read(&c, 1);
            if (n == 1) return c;
            if (n == 0 || !client.connected()) return -1;    // peer closed or socket error
            if (idle && !idle()) {                           // receive timeout: no data yet
                cancelled = failed = true;
                return -1;
            }
        }
        timedOut = failed = true;
        return -1;
    }

    bool line(String &out)
    {
        out = "";
        for (;;) {
            int c = raw();
            if (c < 0) { failed = true; return false; }
            if (c == '\n') return true;
            if (c != '\r' && (out.length() >= 1024 || !out.concat((char)c))) {
                failed = true;
                return false;
            }
        }
    }

    // Reads the start line (request or status line) and all headers.
    bool headers(String &firstLine)
    {
        if (!line(firstLine)) return false;
        if (firstLine.startsWith("HTTP/")) status = firstLine.substring(firstLine.indexOf(' ') + 1).toInt();
        String s;
        size_t headerBytes = firstLine.length();
        while (line(s)) {
            headerBytes += s.length();
            if (headerBytes > 8192) return failed = true, false;
            if (!s.length()) return true;
            int colon = s.indexOf(':');
            if (colon < 0) return failed = true, false;
            String key = s.substring(0, colon), value = s.substring(colon + 1);
            key.toLowerCase();
            value.trim();
            if (key == "content-length") {
                char *end;
                remaining = strtol(value.c_str(), &end, 10);
                if (!value.length() || *end || remaining < 0) return failed = true, false;
            }
            if (key == "transfer-encoding") {
                value.toLowerCase();
                if (value != "chunked") return failed = true, false;
                chunked = true;
            }
            if (key == "content-type") contentType = value;
            if (key == "accept") accept = value;
        }
        return false;
    }

    // Next body byte, or -1 at the end of the body (done) or on error (failed).
    int body()
    {
        if (done || failed) return -1;
        if (chunked) {
            if (!chunkRemaining) {
                if (chunkCRLF && (raw() != '\r' || raw() != '\n')) {
                    failed = true;
                    return -1;
                }
                String sizeLine;
                if (!line(sizeLine)) return -1;
                int semi = sizeLine.indexOf(';');
                if (semi >= 0) sizeLine = sizeLine.substring(0, semi);
                sizeLine.trim();
                char *end;
                unsigned long n = strtoul(sizeLine.c_str(), &end, 16);
                if (!sizeLine.length() || *end || sizeLine[0] == '-') { failed = true; return -1; }
                chunkRemaining = n;
                if (!n) {
                    // Consume optional trailers and the final empty line.
                    unsigned int trailerBytes = 0;
                    do {
                        if (!line(sizeLine)) return -1;
                        trailerBytes += sizeLine.length();
                        if (trailerBytes > 8192) { failed = true; return -1; }
                    } while (sizeLine.length());
                    done = true;
                    return -1;
                }
                chunkCRLF = true;
            }
            int c = raw();
            if (c < 0) { failed = true; return -1; }
            --chunkRemaining;
            return c;
        }
        if (remaining == 0) { done = true; return -1; }
        int c = raw();
        if (c < 0) {
            if (remaining > 0) failed = true;    // closed before Content-Length bytes arrived
            done = !failed;                      // no Content-Length: body ends at close
            return -1;
        }
        if (remaining > 0) --remaining;
        return c;
    }

    bool text(String &out, size_t limit)
    {
        out = "";
        int c;
        while ((c = body()) >= 0) {
            if (out.length() >= limit || !out.concat((char)c)) { failed = true; break; }
        }
        return !failed && done;
    }
};
