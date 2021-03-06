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

static uint16_t cmd[] = {
    // BG2 H/V offset relative:
    2, CMD_BG_OFFSET, 0x0808,
    3, CMD_COLOR_DIRECT_BGR555, COLOR_STROKE, 0x1F3F,
    3, CMD_PIXEL, 18, 118,
    9, CMD_IMAGE, 20, 120, 2, 2,
    0x001F, 0x03E0,
    0x7C00, 0x001F,
    3, CMD_COLOR_DIRECT_BGR555, COLOR_STROKE, 0x7FFF,
    4, CMD_HLINE, 24, 124, 16,
    4, CMD_VLINE, 24, 124, 16,
    3, CMD_COLOR_DIRECT_BGR555, COLOR_STROKE, 0x7C00,
    5, CMD_RECT, 32, 132, 8, 8,
    3, CMD_COLOR_DIRECT_BGR555, COLOR_FILL, 0x001F,
    5, CMD_RECT_FILL, 38, 138, 16, 16,
    4, CMD_COLOR_PALETTED, COLOR_FILL, 0, 0xF0,
    5, CMD_RECT_FILL, 80, 0, 4, 4,
    4, CMD_COLOR_PALETTED, COLOR_FILL, 0, 0xF1,
    5, CMD_RECT_FILL, 84, 0, 4, 4,
    4, CMD_COLOR_PALETTED, COLOR_FILL, 0, 0xF2,
    5, CMD_RECT_FILL, 88, 0, 4, 4,
    4, CMD_COLOR_PALETTED, COLOR_FILL, 0, 0xF3,
    5, CMD_RECT_FILL, 92, 0, 4, 4,
    5, CMD_COLOR_DIRECT_BGR555, COLOR_STROKE, 0x1C4E, COLOR_OUTLINE, 0x0C27,

    // -23
    5, CMD_LINE, static_cast<uint16_t>(-20), static_cast<uint16_t>(242), 260, 9,
    // -17
    7, CMD_COLOR_DIRECT_RGB888, COLOR_STROKE, 0x0000, 0xFF00, COLOR_OUTLINE, 0x00FF, 0x0000,
    // -9
    8, CMD_TEXT_UTF8, 0, 0,
    7, 0, 0, 0, 0,
};
#define cmd_len (sizeof(cmd) / sizeof(uint16_t))

void sleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main(void) {
    uint32_t sent = 0;

#ifdef _WIN32
    setmode(fileno(stdout),O_BINARY);
    setmode(fileno(stdin),O_BINARY);
#endif

    strcpy((char *)&cmd[(cmd_len-4)], "jsd1982");

    // drawlist:
    uint32_t s = 0;
    s = sizeof(cmd);
    sent += printf("WRITE_CORE_MEMORY %lX", (uint32_t) 0xFF020000);
    sent += printf(" %02X %02X", s & 0xFF, (s >> 8) & 0xFF);
    for (int i = 0; i < s; i++) {
        auto c = ((uint8_t*)cmd)[i];
        sent += printf(" %02X", c);
    }
    sent += printf("\n");
    sleep(17);
    sent = 0;

    // jump table:
    sent += printf("WRITE_CORE_MEMORY %lX", (uint32_t) 0xFFFFE000);
    uint16_t x_offs = 2304;
    uint16_t y_offs = 8464;
    sent += printf(" %02X %02X %02X %02X %02X %02X %02X %02X",
        1 & 0xFF, (1 >> 8) & 0xFF,              // drawlist 1:
        OAM | 0x80,                             // layer (draw to MAIN not SUB)
        3,                                      // priority
        x_offs & 0xFF, (x_offs >> 8) & 0xFF,    // x_offset
        y_offs & 0xFF, (y_offs >> 8) & 0xFF     // y_offset
    );
    // end of list:
    sent += printf(" %02X %02X", 0 & 0xFF, (0 >> 8) & 0xFF);
    sent += printf("\n");
    sleep(17);
    sent = 0;
}