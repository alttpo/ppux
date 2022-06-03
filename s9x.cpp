// MUST be #included from snes9x gfx.cpp after all existing header #includes

#include "drawlist_fwd.hpp"
#include "drawlist.hpp"

#include "drawlist_render.hpp"

using namespace DrawList;

struct LayerPlot {
    LayerPlot(DrawList::draw_layer p_layer, uint8_t p_priority) : priority(p_priority) {}

    uint8_t priority;

    void operator() (int x, int y, uint16_t color) {
        // draw to any PPU layer:
        auto offs = (y << 8) + x;
        if (priority > GFX.DB[offs]) {
            GFX.S[offs] = color;
            GFX.DB[offs] = priority;
        }
    }
};

using LayerRenderer = DrawList::GenericRenderer<256, 256, LayerPlot>;

void PPUXUpdate(void) {
    static uint16_t cmd[] = {
            4, CMD_TARGET, OAM, 0, 15,
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
            5, CMD_LINE, 10, static_cast<uint16_t>(-12), 260, 239,
            // -17
            7, CMD_COLOR_DIRECT_RGB888, COLOR_STROKE, 0x0000, 0xFF00, COLOR_OUTLINE, 0x00FF, 0x0000,
            // -9
            8, CMD_TEXT_UTF8, 0, 0,
            7, 0, 0, 0, 0,
    };
#define cmd_len (sizeof(cmd) / sizeof(uint16_t))
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

    Context c(
        [](draw_layer i_layer, bool i_pre_mode7_transform, uint8_t i_priority, std::shared_ptr<Renderer>& o_target){
            o_target = (std::shared_ptr<Renderer>) std::make_shared<LayerRenderer>(i_layer, i_priority);
        },
        std::make_shared<FontContainer>(),
        std::make_shared<SpaceContainer>(
            std::make_shared<LocalSpace>(Memory.VRAM, reinterpret_cast<uint8_t *>(PPU.CGDATA))
        )
    );

    std::vector<uint16_t> cmdlist(cmd, cmd + cmd_len);
    c.draw_list(cmdlist);
}
