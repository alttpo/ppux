#pragma once

#include "drawlist_fwd.hpp"
#include "drawlist.hpp"

#include "drawlist_render.hpp"

extern std::shared_ptr<DrawList::SpaceContainer> spaceContainer;
extern std::shared_ptr<DrawList::FontContainer> fontContainer;

extern uint8_t  drawlistSize[2];
extern uint8_t  drawlistBuffer[0x20000 - 2];

extern uint16_t spaceVRAM[0x8000 * DrawList::SpaceContainer::MaxCount-1];
extern uint16_t spaceCGRAM[0x100 * DrawList::SpaceContainer::MaxCount-1];
