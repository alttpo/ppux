// must be included by libretro.cpp

#include "s9x.h"

#define PPUX_MEM_DESC_COUNT 4

// assumes mem is allocated with at least PPUX_MEM_DESC_COUNT descriptors
// and that mem is pointing at the first descriptor to fill in
void PPUXFillMemoryDescriptors(struct retro_memory_descriptor* mem) {
    mem[0].start = 0xFFFFFFFC;
    mem[0].len = 4;
    mem[0].ptr = &drawlistSize;
    mem[0].addrspace = "XL"; // PPUX Length

    mem[1].start = 0xFFFE0000;
    mem[1].len = sizeof(drawlistBuffer) - 4;
    mem[1].ptr = drawlistBuffer;
    mem[1].addrspace = "XD"; // PPUX DrawList Buffer

    mem[2].start = 0xF0000000;
    mem[2].len = 0x10000 * sizeof(uint16_t) * DrawList::SpaceContainer::MaxCount;
    mem[2].ptr = spaceVRAM;
    mem[2].addrspace = "XV"; // PPUX VRAM extra

    mem[3].start = 0xFF000000;
    mem[3].len =   0x100 * sizeof(uint16_t) * DrawList::SpaceContainer::MaxCount;
    mem[3].ptr = spaceCGRAM;
    mem[3].addrspace = "XC"; // PPUX CGRAM extra
}
