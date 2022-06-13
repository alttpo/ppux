// MUST be #included from snes9x gfx.cpp after all existing header #includes

#include "s9x.h"

using namespace DrawList;

std::shared_ptr<SpaceContainer> spaceContainer;
std::shared_ptr<FontContainer> fontContainer;

uint8_t  drawlistSize[4];
uint8_t  drawlistBuffer[0x20000 - 4];

uint16_t spaceVRAM[0x8000 * DrawList::SpaceContainer::MaxCount-1];
uint16_t spaceCGRAM[0x100 * DrawList::SpaceContainer::MaxCount-1];

#ifdef PPUX_TEST_PATTERN
static uint16_t cmd[] = {
        3, CMD_TARGET, OAM, 3,
        // BG2 H/V offset relative:
        2, CMD_BG_OFFSET, 0x0808,
        // Position at upper-left of overworld Link's house:
        3, CMD_XY_OFFSET, 2048, 2560,
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
#endif

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
                default: return D;
            }

        case 7:
        default:
            return D;
    }
}

template<int width, int height>
struct LayerPlot {
    LayerPlot(State& p_state, bool p_sub)
      : state(p_state), depth(PPUXCalcDepth(p_state.layer, p_state.priority, p_sub))
    {
    }

    State& state;
    uint8_t depth;

    void operator() (int x, int y, uint16_t color) {
        // coords are already xOffset/yOffset adjusted by Renderer implementation
        if (!bounds_check<width, height>(x, y)) {
            return;
        }

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

using LayerRenderer = DrawList::GenericRenderer<256, 256, LayerPlot<256, 256>>;

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
#ifdef PPUX_TEST_PATTERN
    std::vector<uint16_t> cmdlist(cmd, cmd + cmd_len);
#else
    // little endian conversion:
    uint32_t len =
        ((uint32_t)drawlistSize[0]) |
        ((uint32_t)drawlistSize[1] << 8) |
        ((uint32_t)drawlistSize[2] << 16) |
        ((uint32_t)drawlistSize[3] << 24);

    if (len == 0)
        return;

    if (len > sizeof(drawlistBuffer)) {
        fprintf(stderr, "PPUXRender: drawlist size (%u) too large for buffer size (%u)\n", len, sizeof(drawlistBuffer));
        return;
    }

    std::vector<uint16_t> cmdlist((uint16_t*)drawlistBuffer, (uint16_t*)(drawlistBuffer + len));
#endif

    Context c(
        [=](State& state, std::shared_ptr<Renderer>& o_target){
            o_target = (std::shared_ptr<Renderer>) std::make_shared<LayerRenderer>(
                state,
                LayerPlot<256, 256>(state, (bool)sub)
            );
        },
        [=](draw_layer layer, int& xOffset, int& yOffset) {
            xOffset = PPU.BG[layer].HOffset;
            yOffset = PPU.BG[layer].VOffset;
        },
        fontContainer,
        spaceContainer
    );

    c.draw_list(cmdlist);
}

void PPUXUpdate() {
#ifdef PPUX_TEST_PATTERN
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
#endif
}
