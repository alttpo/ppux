#pragma once

#include "drawlist_fwd.hpp"
#include "drawlist.hpp"

#include "drawlist_render.hpp"

extern std::shared_ptr<DrawList::SpaceContainer> spaceContainer;
extern std::shared_ptr<DrawList::FontContainer> fontContainer;

struct leu16 {
    uint8_t b[2];
    alwaysinline uint16_t u16() const { return (uint16_t)b[0] | ((uint16_t)b[1])<<8; }
};

const int drawlistCount = 1024;
struct drawlist_jump_table {
    // 0000 index signifies end of jump table; values in range [1..drawlistCount]
    leu16 index;
    uint8_t target_layer;
    uint8_t target_priority;
    leu16 x_offset;
    leu16 y_offset;
};
extern drawlist_jump_table drawlistJump[drawlistCount];

struct drawlist_data {
    leu16 size;
    uint8_t data[2048 - 2];
};
extern drawlist_data drawlists[drawlistCount];

struct font_data {
    // fonts begin with 3 byte little endian size followed by data, repeating size+data until size==0
    uint8_t data[0xDDE000];
};
extern font_data fonts;

extern uint16_t spaceVRAM[0x8000 * DrawList::SpaceContainer::MaxCount-1];
extern uint16_t spaceCGRAM[0x100 * DrawList::SpaceContainer::MaxCount-1];
