// MUST be #included from snes9x gfx.cpp after all existing header #includes

#include "s9x.h"

using namespace DrawList;

std::shared_ptr<SpaceContainer> spaceContainer;
std::shared_ptr<FontContainer> fontContainer;

drawlist_jump_table drawlistJump;
drawlist_data drawlists[drawlistCount];
font_data fonts;

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
    memset((void*)&drawlists, 0, sizeof(drawlists));
    memset((void*)&fonts, 0, sizeof(fonts));
    memset((void*)&drawlistJump, 0, sizeof(drawlistJump));
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

    // check if new fonts ready to load:
    auto fontCountNew = fonts.data[0];
    if (fontCountNew != fontContainer->size()) {
        // clear the count so we don't re-read it next time unless it changes:
        fonts.data[0] = 0;

        uint8_t* p = fonts.data;
        for (int i = 0; i < fontCountNew; i++) {
            // read size of font as 3 bytes little-endian:
            int size = ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);

            // clear the size so we don't re-read it next time:
            p[0] = 0;
            p[1] = 0;
            p[2] = 0;
            p += 3;

            if (size == 0) {
                break;
            }

            const uint8_t *data = p;
            p += size;

            // load the PCF font data:
            fontContainer->load_pcf(i, data, size);
        }
    }

    // iterate through the jump table and draw the lists:
    auto endp = (uint16_t *)drawlistJump.index + drawlistCount;
    // TODO: replace *p with little-endian uint16_t reads
    for (auto p = (uint16_t *)drawlistJump.index; p != endp && *p != 0; p++) {
        auto index = *p - 1;
        if (index >= drawlistCount)
            break;

        drawlist_data &dl = drawlists[index];

        uint32_t  end = ((uint16_t)dl.size[0]) | ((uint16_t)dl.size[1] << 8);
        if (end == 0)
            continue;

        if (end > sizeof(dl.data)) {
            fprintf(stderr, "PPUXRender: drawlist[%u] size (%u) too large for buffer size (%u)\n", *p, end, sizeof(dl.data));
            return;
        }

        auto start = (uint16_t*) dl.data;
        c.draw_list(start, end);
    }
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
