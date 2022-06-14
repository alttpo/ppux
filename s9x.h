#pragma once

#include "drawlist_fwd.hpp"
#include "drawlist.hpp"

#include "drawlist_render.hpp"

extern std::shared_ptr<DrawList::SpaceContainer> spaceContainer;
extern std::shared_ptr<DrawList::FontContainer> fontContainer;

const int            drawlistCount = 1024;
struct drawlist_jump_table {
    // 0000 index signifies end of jump table
    uint8_t index[drawlistCount][2];
};
extern drawlist_jump_table drawlistJump;

struct drawlist_data {
    uint8_t size[2];
    uint8_t data[2048 - 2];
};
extern drawlist_data drawlists[drawlistCount];

struct font_data {
    // fonts begin with 3 byte little endian size followed by data, repeating size+data until size==0
    uint8_t data[0xDDF800];
};
extern font_data fonts;

extern uint16_t spaceVRAM[0x8000 * DrawList::SpaceContainer::MaxCount-1];
extern uint16_t spaceCGRAM[0x100 * DrawList::SpaceContainer::MaxCount-1];
