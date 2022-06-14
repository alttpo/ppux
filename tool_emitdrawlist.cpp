#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "drawlist_fwd.hpp"

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

int main(void) {
    strcpy((char *)&cmd[(cmd_len-4)], "jsd1982");

    // drawlist:
    uint32_t s = sizeof(cmd);
    printf("WRITE_CORE_MEMORY %lX", (uint32_t) 0xFF020000);
    printf(" %02X %02X", s & 0xFF, (s >> 8) & 0xFF);
    for (int i = 0; i < s; i++) {
        auto c = ((uint8_t*)cmd)[i];
        printf(" %02X", c);
    }
    printf("\n");

    // jump table:
    printf("WRITE_CORE_MEMORY %lX", (uint32_t) 0xFFFFE000);
    printf(" %02X %02X %02X %02X %02X %02X %02X %02X",
        1 & 0xFF, (1 >> 8) & 0xFF,          // drawlist 1:
        OAM,                                // layer
        3,                                  // priority
        // Uncle Passage:
        2560 & 0xFF, (2560 >> 8) & 0xFF,    // x_offset
        2560 & 0xFF, (2560 >> 8) & 0xFF     // y_offset
    );
    // end of list:
    printf(" %02X %02X", 0 & 0xFF, (0 >> 8) & 0xFF);
    printf("\n");
}