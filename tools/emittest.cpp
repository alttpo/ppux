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
    d = cmd;
    d += 2;
    d = emit_string(d, "bg1p0");
    s = (d - cmd) * sizeof(uint16_t);
    // write the size:
    d = cmd;
    *d++ = s & 0xFF;
    *d++ = (s >> 8) & 0xFF;

    // send:
    write_core_memory(0xFF020000, (uint8_t*)cmd, s);

    // jump table:
    uint8_t  jt[32768];
    uint8_t *jd = jt;
    uint32_t js = 0;
    for (uint16_t y_offs = 0; y_offs < 240; y_offs += 12) {
        for (uint16_t x_offs = 0; x_offs < 256; x_offs += 24) {
            *jd++ = 1 & 0xFF;
            *jd++ = (1 >> 8) & 0xFF;         // drawlist 1
            *jd++ = BG1 | 0x80;              // layer (draw to MAIN not SUB)
            *jd++ = 0;                       // priority
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
    write_core_memory(0xFFFFE000, jt, js);
}