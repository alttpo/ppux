// MUST be #included from snes9x gfx.cpp after all existing header #includes

#include "s9x.h"

using namespace DrawList;

std::shared_ptr<SpaceContainer> spaceContainer;
std::shared_ptr<FontContainer> fontContainer;

uint8_t  drawlistSize[2];
uint8_t  drawlistBuffer[0x20000 - 2];

uint16_t spaceVRAM[0x10000 * DrawList::SpaceContainer::MaxCount-1];
uint16_t spaceCGRAM[ 0x200 * DrawList::SpaceContainer::MaxCount-1];

// layer is one of `(BG1,BG2,BG3,OAM,BACK)`
// priority is 0 or 1 for BG layers, and 0..3 for OAM layer
uint8_t PPUXCalcDepth(DrawList::draw_layer layer, uint8_t priority, bool sub) {
    int D;

    if (!sub)
    {
        D = 32;
    }
    else
    {
        D = (Memory.FillRAM[0x2130] & 2) << 4; // 'do math' depth flag
    }

    if (layer == DrawList::OAM)
    {
        return (D + 4) + (priority * 4);
    }
    else if (layer >= DrawList::COL)
    {
        return D;
    }

    switch (PPU.BGMode)
    {
        case 0:
            switch (layer) {
                case DrawList::BG1: return priority ? D + 15 : D + 11;
                case DrawList::BG2: return priority ? D + 14 : D + 10;
                case DrawList::BG3: return priority ? D +  7 : D +  3;
                case DrawList::BG4: return priority ? D +  6 : D +  2;
                default: return D;
            }

        case 1:
            switch (layer) {
                case DrawList::BG1: return priority ? D + 15 : D + 11;
                case DrawList::BG2: return priority ? D + 14 : D + 10;
                case DrawList::BG3: return priority ? D + (PPU.BG3Priority ? 17 : 7) : D +  3;
                default: return D;
            }

        case 2:
        case 3:
        case 4:
        case 5:
            switch (layer) {
                case DrawList::BG1: return priority ? D + 15 : D + 7;
                case DrawList::BG2: return priority ? D + 11 : D + 3;
                default: return D;
            }

        case 6:
            switch (layer) {
                case DrawList::BG1: return priority ? D + 15 : D + 7;
                default: return D ;
            }

        case 7:
        default:
            return D;
    }
}

struct LayerPlot {
    LayerPlot(DrawList::draw_layer p_layer, uint8_t p_priority, bool p_sub)
      : depth(PPUXCalcDepth(p_layer, p_priority, p_sub))
    {}

    uint8_t depth;

    void operator() (int x, int y, uint16_t color) {
        // draw to any PPU layer:
        auto offs = (y * GFX.PPL) + x;
        // compare depth:
        if (depth >= GFX.DB[offs]) {
            // convert BGR555 to RGB565 (or whatever PIXEL_FORMAT is):
            GFX.S[offs] = BUILD_PIXEL(
                IPPU.XB[color & 0x1F],          // red
                IPPU.XB[(color >> 5) & 0x1F],   // green
                IPPU.XB[(color >> 10) & 0x1F]   // blue
            );
            GFX.DB[offs] = depth;
        }
    }
};

using LayerRenderer = DrawList::GenericRenderer<256, 256, LayerPlot>;

static uint16_t cmd[] = {
    3, CMD_TARGET, OAM, 3,
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
    5, CMD_LINE, static_cast<uint16_t>(-10), static_cast<uint16_t>(-12), 260, 239,
    // -17
    7, CMD_COLOR_DIRECT_RGB888, COLOR_STROKE, 0x0000, 0xFF00, COLOR_OUTLINE, 0x00FF, 0x0000,
    // -9
    8, CMD_TEXT_UTF8, 0, 0,
    7, 0, 0, 0, 0,
};
#define cmd_len (sizeof(cmd) / sizeof(uint16_t))

void PPUXInit() {
    memset(drawlistSize, 0, sizeof(drawlistSize));
    memset(drawlistBuffer, 0, sizeof(drawlistBuffer));
    memset(spaceVRAM, 0, sizeof(spaceVRAM));
    memset(spaceCGRAM, 0, sizeof(spaceCGRAM));

    fontContainer = std::make_shared<FontContainer>();
    spaceContainer = std::make_shared<SpaceContainer>(
        std::make_shared<LocalSpace>(Memory.VRAM, reinterpret_cast<uint8_t *>(PPU.CGDATA)),
        [](int index) {
            return std::make_shared<LocalSpace>(
                (uint8_t*)spaceVRAM + index*0x10000*sizeof(uint16_t),
                (uint8_t*)spaceCGRAM + index*0x200*sizeof(uint16_t)
            );
        }
    );
}

void PPUXRender(bool8 sub) {
    // big endian conversion:
    uint16_t len = ((uint16_t)drawlistSize[0] << 8) | (uint16_t)drawlistSize[1];
    if (len == 0)
        return;

    Context c(
        [=](draw_layer i_layer, uint8_t i_priority, std::shared_ptr<Renderer>& o_target){
            o_target = (std::shared_ptr<Renderer>) std::make_shared<LayerRenderer>(i_layer, i_priority, (bool)sub);
        },
        fontContainer,
        spaceContainer
    );

    //std::vector<uint16_t> cmdlist(cmd, cmd + cmd_len);
    std::vector<uint16_t> cmdlist(drawlistBuffer, drawlistBuffer + len);
    c.draw_list(cmdlist);
}

void PPUXUpdate() {
    strcpy((char *)&cmd[(cmd_len-4)], "jsd1982");

    // increment y:
    cmd[cmd_len-6]++;
    if (cmd[cmd_len-6] == 240) {
        cmd[cmd_len-6] = -12;
        // increment x:
        cmd[cmd_len-7] += 4;
        if (cmd[cmd_len-7] == 240) {
            cmd[cmd_len-7] = 0;
        }
    }

    // CMD_LINE y:
    cmd[cmd_len-18]++;
    if (cmd[cmd_len-18] >= 240) {
        cmd[cmd_len-18] = 0;
    }

    cmd[cmd_len-20]--;
    if ((int16_t)cmd[cmd_len-20] <= -12) {
        cmd[cmd_len-20] = 240;
    }
}