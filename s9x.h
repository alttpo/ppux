#pragma once

#include "drawlist_fwd.hpp"
#include "drawlist.hpp"

#include "drawlist_render.hpp"

extern std::shared_ptr<DrawList::SpaceContainer> spaceContainer;
extern std::shared_ptr<DrawList::FontContainer> fontContainer;

extern uint16_t drawlistSize;
extern uint16_t drawlistBuffer[0x10000];

extern uint8_t spaceVRAM[0x10000 * 1024];
extern uint8_t spaceCGRAM[0x200 * 1024];
