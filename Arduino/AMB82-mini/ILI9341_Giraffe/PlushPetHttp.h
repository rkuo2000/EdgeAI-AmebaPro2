#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <stdlib.h>

// HTTP framing shared by AI responses, TTS downloads and the control API.
// A single absolute deadline prevents a stalled peer from holding the pet forever.
class PetHttpReader {
public:
    WiFiClient &client;
    uint32_t started, timeout;
    int status = 0;
    long remaining = -1;
    bool chunked = false, failed = false, done = false;
    size_t chunkRemaining = 0;
    bool chunkCRLF = false;
    String contentType;

    PetHttpReader(WiFiClient &c, uint32_t t): client(c), started(millis()), timeout(t) {}
    int raw() {
        // The SDK's available() spins forever on an idle blocking socket (e.g. a
        // browser preconnect). read() waits at most the 3 s socket receive timeout.
        while (millis() - started < timeout) {
            uint8_t c;
            int n = client.read(&c, 1);
            if (n == 1) return c;
            if (n == 0 || !client.connected()) return -1;
        }
        failed = true;
        return -1;
    }
    bool line(String &out) {
        out = "";
        for (;;) {
            int c = raw();
            if (c < 0) { failed = true; return false; }
            if (c == '\n') return true;
            if (c != '\r' && (out.length() >= 1024 || !out.concat((char)c))) {
                failed = true; return false;
            }
        }
    }
    bool headers(String &firstLine) {
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
            key.toLowerCase(); value.trim();
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
        }
        return false;
    }
    int body() {
        if (done || failed) return -1;
        if (chunked) {
            if (!chunkRemaining) {
                if (chunkCRLF && (raw() != '\r' || raw() != '\n')) {
                    failed = true; return -1;
                }
                String sizeLine;
                if (!line(sizeLine)) return -1;
                int semi = sizeLine.indexOf(';');
                if (semi >= 0) sizeLine = sizeLine.substring(0, semi);
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
                    done = true; return -1;
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
            if (remaining > 0) failed = true;
            done = !failed;
            return -1;
        }
        if (remaining > 0) --remaining;
        return c;
    }
    bool text(String &out, size_t limit) {
        out = "";
        int c;
        while ((c = body()) >= 0) {
            if (out.length() >= limit || !out.concat((char)c)) { failed = true; break; }
        }
        return !failed && done;
    }
};

inline bool petWriteAll(WiFiClient &client, const char *data, size_t size) {
    uint32_t progress = millis();
    while (size) {
        if (!client.connected() || millis() - progress > 15000UL) return false;
        size_t n = client.write((const uint8_t *)data, size > 1024 ? 1024 : size);
        if (n) { data += n; size -= n; progress = millis(); }
        else delay(1);
    }
    return true;
}
