// MUST be #included from snes9x gfx.cpp after all existing header #includes

#include "s9x.h"

using namespace DrawList;

std::shared_ptr<SpaceContainer> spaceContainer;
std::shared_ptr<FontContainer> fontContainer;

drawlist_jump_table drawlistJump[drawlistCount];
drawlist_data drawlists[drawlistCount];
font_data fonts;

uint16_t spaceVRAM[0x8000 * DrawList::SpaceContainer::MaxCount-1];
uint16_t spaceCGRAM[0x100 * DrawList::SpaceContainer::MaxCount-1];

const int maxScreenSize = (256*2)*(240*2);
uint16 ppuxMainColor[maxScreenSize];
uint8  ppuxMainDepth[maxScreenSize];
uint8  ppuxMainLayer[maxScreenSize];
uint16 ppuxSubColor[maxScreenSize];
uint8  ppuxSubDepth[maxScreenSize];
uint8  ppuxSubLayer[maxScreenSize];

template<int width, int height>
struct LayerPlot {
    explicit LayerPlot(Target& p_target)
      : target(p_target), depthMain(31), depthSub(63)
    {
    }

    Target& target;
    uint8_t depthMain, depthSub;

    void target_updated() {
        depthMain = calcDepth(target.layer, target.priority, false);
        depthSub = calcDepth(target.layer, target.priority, true);
    }

    // layer is one of `(BG1,BG2,BG3,OAM,BACK)`
    // priority is 0 or 1 for BG layers, and 0..3 for OAM layer
    static uint8_t calcDepth(DrawList::draw_layer layer, uint8_t priority, bool sub) {
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

    void operator() (int x, int y, uint16_t color) {
        // coords are already xOffset/yOffset adjusted by Renderer implementation
        // coords MUST be bounds checked before calling plot()
        assert( (bounds_check<width, height>(x, y)) );

        // draw to any PPU layer:
        auto offs = (y * GFX.PPL) + x;

        // draw to main:
        if (target.main_enable && depthMain >= ppuxMainDepth[offs]) {
            ppuxMainColor[offs] = BUILD_PIXEL(
                IPPU.XB[color & 0x1F],          // red
                IPPU.XB[(color >> 5) & 0x1F],   // green
                IPPU.XB[(color >> 10) & 0x1F]   // blue
            );
            ppuxMainDepth[offs] = depthMain;
            ppuxMainLayer[offs] = target.layer;
        }
        // draw to sub:
        if (target.sub_enable && depthSub >= ppuxSubDepth[offs]) {
            ppuxSubColor[offs] = BUILD_PIXEL(
                IPPU.XB[color & 0x1F],          // red
                IPPU.XB[(color >> 5) & 0x1F],   // green
                IPPU.XB[(color >> 10) & 0x1F]   // blue
            );
            ppuxSubDepth[offs] = depthSub;
            ppuxSubLayer[offs] = target.layer;
        }
    }
};

using LayerRenderer = DrawList::GenericRenderer<256, 256, LayerPlot<256, 256>>;

State ppuxState;
Target ppuxTarget;
std::unique_ptr<Context> ppuxContext;
LayerPlot<256, 256> ppuxPlot(ppuxTarget);

void PpuxInit() {
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

    ppuxContext = std::make_unique<Context>(
        ppuxState,
        ppuxTarget,
        (std::shared_ptr<Renderer>) std::make_shared<LayerRenderer>(
            ppuxState,
            ppuxPlot
        ),
        [](draw_layer layer, int& xOffset, int& yOffset) {
            xOffset = PPU.BG[layer].HOffset;
            yOffset = PPU.BG[layer].VOffset;
        },
        fontContainer,
        spaceContainer
    );
}

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

void PpuxLoadFonts() {
    // check if new fonts ready to load:
    if (fonts.do_load == 0) {
        return;
    }

    //printf("  loadFonts()\n");

    // clear the count so we don't re-read it next time unless it changes:
    fonts.do_load = 0;

    // clear the font container and load in new fonts:
    fontContainer->clear();

    int fontIndex = 0;
    uint8_t* p = fonts.data;
    uint8_t* endp = fonts.data + sizeof(fonts.data);
    while (p < endp) {
        // read size of font as 3 bytes little-endian:
        auto size = ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);

        // clear the size so we don't re-read it next time:
        p[0] = 0;
        p[1] = 0;
        p[2] = 0;
        p += 3;

        // stopping condition:
        if (size == 0) {
            break;
        }
        if (p >= endp) {
            fprintf(stderr, "PPUXRender: not enough buffer space to load font data at position %lu\n", p - fonts.data);
            break;
        }
        if (p + size > endp) {
            fprintf(stderr, "PPUXRender: font size %u at position %lu reaches beyond the buffer\n", size, p - fonts.data);
            break;
        }

        uint8_t* data = p;
        p += size;

        // load the PCF font data:
        fontContainer->load_pcf(fontIndex++, data, (int)size);

        // clear the font data:
        memset(data, 0, size);
    }
}

void PpuxStartFrame() {
    bool cleared = false;

    //printf("SOF\n");

    PpuxLoadFonts();

    // iterate through the jump table and draw the lists:
    for (int i = 0; i < drawlistCount; i++) {
        auto & jump = drawlistJump[i];

        // index is in range [1..drawlistCount]:
        auto index = jump.index.u16();
        if (index == 0)
            break;
        if (index > drawlistCount) {
            fprintf(stderr, "PPUXRender: bad drawlist index %u at position %u; must be in range [1..%u]\n", index, i, drawlistCount);
            break;
        }

        // subtract one for array indexing:
        drawlist_data &dl = drawlists[index - 1];

        uint32_t size = dl.size.u16();
        if (size == 0)
            continue;

        if (size > sizeof(dl.data)) {
            fprintf(stderr, "PPUXRender: drawlist[%u] size %u too large for buffer size %u\n", index, size, sizeof(dl.data));
            return;
        }

        if (!cleared) {
            // shouldn't need to clear color buffers here since depth should be enough:
            memset(ppuxMainDepth, 0, sizeof(ppuxMainDepth));
            memset(ppuxSubDepth, 0, sizeof(ppuxSubDepth));
            cleared = true;
        }

        auto start = (uint16_t*) dl.data;
        ppuxTarget.layer = jump.layer();
        ppuxTarget.priority = jump.priority();
        ppuxTarget.main_enable = jump.main_enable();
        ppuxTarget.sub_enable = jump.sub_enable();
        ppuxState.reset_state();
        ppuxState.xOffsetXY = jump.x_offset.u16();
        ppuxState.yOffsetXY = jump.y_offset.u16();
        ppuxContext->draw_list(start, size);
        //printf("  [%3d] drawlist(%d)\n", IPPU.CurrentLine, index);
    }
}

void PpuxEndFrame() {
    //printf("EOF\n");
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

void PpuxRenderLine(int layer, bool8 sub) {
    static const char* layerNames[6] = {
        "BG1",
        "BG2",
        "BG3",
        "BG4",
        "OAM",
        "COL"
    };

    // merge with main and sub screens with window clipping and colormath:
    uint16 *c;
    uint8  *d;
    uint16 *xc;
    uint8  *xd;
    uint8  *xr;
    struct ClipData *p;

    // select either main or sub clip structure:
    p = IPPU.Clip[sub & 1];

    // draw into the main or sub screen:
    for (int clip = 0; clip < p[layer].Count; clip++) {
        if (p[layer].DrawMode[clip] == 0) {
            continue;
        }

        if (!sub) {
            // main:
            c = GFX.Screen + (GFX.StartY * GFX.PPL);
            d = GFX.ZBuffer + (GFX.StartY * GFX.PPL);
            xc = ppuxMainColor + (GFX.StartY * GFX.PPL);
            xd = ppuxMainDepth + (GFX.StartY * GFX.PPL);
            xr = ppuxMainLayer + (GFX.StartY * GFX.PPL);
        } else {
            // sub:
            c = GFX.SubScreen + (GFX.StartY * GFX.PPL);
            d = GFX.SubZBuffer + (GFX.StartY * GFX.PPL);
            xc = ppuxSubColor + (GFX.StartY * GFX.PPL);
            xd = ppuxSubDepth + (GFX.StartY * GFX.PPL);
            xr = ppuxSubLayer + (GFX.StartY * GFX.PPL);
        }

        //printf("  %s[%3d] clip[%1d] Y:%3d..%3d, X:%3d..%3d\n", layerNames[layer], IPPU.CurrentLine, clip, GFX.StartY, GFX.EndY, p[layer].Left[clip], p[layer].Right[clip]);

        for (uint32 l = GFX.StartY; l <= GFX.EndY; l++, c += GFX.PPL, d += GFX.PPL, xc += GFX.PPL, xd += GFX.PPL, xr += GFX.PPL) {
            for (int x = p[layer].Left[clip]; x < p[layer].Right[clip]; x++) {
                // TODO: colormath
                if (xr[x] == layer && xd[x] >= d[x]) {
                    c[x] = xc[x];
                    d[x] = xd[x];
                }
            }
        }
    }
}
