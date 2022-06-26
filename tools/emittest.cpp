#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include <chrono>
#include <thread>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../drawlist_fwd.hpp"

using namespace DrawList;

void sleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

uint16_t *emit_string(uint16_t *c, const char *s) {
    auto len = strlen(s);
    auto wlen = (len+(len&1))/2;
    *c++ = 4 + wlen;
    *c++ = CMD_TEXT_UTF8;
    *c++ = 0;
    *c++ = 0;
    *c++ = len;
    strcpy((char *)c, s);
    c += wlen;
    return c;
}

uint32_t sent = 0;
uint32_t total = 0;

void write_core_memory(uint32_t a, uint8_t *d, uint32_t s) {
    uint32_t w = 0;
    sent += printf("WRITE_CORE_MEMORY %lX", a);
    while (w < s) {
        auto c = *d++;
        a++;
        w++;
        sent += printf(" %02X", c);

        if (sent >= 1020) {
            sent += printf("\n");
            total += sent;
            sent = 0;
            if (total >= 8180) {
                sleep(17);
                total = 0;
            }
            if (w < s) {
                sent += printf("WRITE_CORE_MEMORY %lX", a);
            }
        }
    }
    if (sent > 0) {
        sent += printf("\n");
    }
    total += sent;
}

int main(void) {
    uint16_t cmd[2048];
    uint16_t *d;
    uint32_t s;

    sent = 0;
    total = 0;

#ifdef _WIN32
    setmode(fileno(stdout),O_BINARY);
    setmode(fileno(stdin),O_BINARY);
#endif

    // build the test drawlists:
    uint32_t da = 0xFF020000;
    uint16_t *dl = cmd;
    d = dl + 1;
    d = emit_string(d, "bg2p0");
    // write the size:
    *dl = (d - (dl + 1)) * sizeof(uint16_t);

    // send:
    write_core_memory(da, (uint8_t*)cmd, (d - cmd) * sizeof(uint16_t));
    da += 2048;

    dl = cmd;
    d = dl + 1;
    d = emit_string(d, "bg2p1");
    // write the size:
    *dl = (d - (dl + 1)) * sizeof(uint16_t);

    // send:
    write_core_memory(da, (uint8_t*)cmd, (d - cmd) * sizeof(uint16_t));

    // jump table:
    uint8_t  jt[32768];
    uint8_t *jd = jt;
    uint32_t js = 0;
    for (uint16_t y_offs = 0; y_offs <= 240 - 12; y_offs += 24) {
        for (uint16_t x_offs = 0; x_offs <= 256 - 32; x_offs += 32) {
            *jd++ = 1 & 0xFF;
            *jd++ = (1 >> 8) & 0xFF;         // drawlist 1
            *jd++ = BG2 | 0x80;              // layer (draw to MAIN not SUB)
            *jd++ = 0;                       // priority
            *jd++ = x_offs & 0xFF;
            *jd++ = (x_offs >> 8) & 0xFF;    // x_offset
            *jd++ = y_offs & 0xFF;
            *jd++ = (y_offs >> 8) & 0xFF;    // y_offset
            js += 8;
        }
    }
    for (uint16_t y_offs = 12; y_offs <= 240 - 12; y_offs += 24) {
        for (uint16_t x_offs = 0; x_offs <= 256 - 32; x_offs += 32) {
            *jd++ = 2 & 0xFF;
            *jd++ = (2 >> 8) & 0xFF;         // drawlist 1
            *jd++ = BG2 | 0x80;              // layer (draw to MAIN not SUB)
            *jd++ = 1;                       // priority
            *jd++ = x_offs & 0xFF;
            *jd++ = (x_offs >> 8) & 0xFF;    // x_offset
            *jd++ = y_offs & 0xFF;
            *jd++ = (y_offs >> 8) & 0xFF;    // y_offset
            js += 8;
        }
    }
    // end of list:
    *jd++ = 0 & 0xFF;
    *jd++ = (0 >> 8) & 0xFF;
    js += 2;
    write_core_memory(0xFFFFE000, jt, js);
}