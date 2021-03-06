// must be included by libretro.cpp

#include "s9x.h"

#define PPUX_MEM_DESC_COUNT 5

// assumes mem is allocated with at least PPUX_MEM_DESC_COUNT descriptors
// and that mem is pointing at the first descriptor to fill in
void PPUXFillMemoryDescriptors(struct retro_memory_descriptor* mem) {
    // 0x4000000 total bytes (64MiB)

    mem[0].start = 0xFE000000; // .. 0xFEFF_FFFF
    mem[0].len = 0x8000 * sizeof(uint16_t) * DrawList::SpaceContainer::MaxCount;
    mem[0].ptr = (void*)&spaceVRAM;
    mem[0].addrspace = "XV"; // PPUX VRAM extra

    mem[1].start = 0xFF000000; // .. 0xFF01_FFFF
    mem[1].len =   0x100 * sizeof(uint16_t) * DrawList::SpaceContainer::MaxCount;
    mem[1].ptr = (void*)&spaceCGRAM;
    mem[1].addrspace = "XC"; // PPUX CGRAM extra

    mem[2].start = 0xFF020000; // .. 0xFF21_FFFF
    mem[2].len = sizeof(drawlists);
    mem[2].ptr = (void*)&drawlists;
    mem[2].addrspace = "XD"; // PPUX DrawLists Data

    mem[3].start = 0xFF220000; // .. 0xFFFF_DFFF
    mem[3].len = sizeof(fonts);
    mem[3].ptr = (void*)&fonts;
    mem[3].addrspace = "XF"; // PPUX PCF fonts

    mem[4].start = 0xFFFFE000; // .. 0xFFFF_FFFF
    mem[4].len = sizeof(drawlistJump);
    mem[4].ptr = (void*)&drawlistJump;
    mem[4].addrspace = "XJ"; // PPUX DrawList Jump Table

    // ensure no overlaps:
    assert(mem[0].start + mem[0].len <= mem[1].start);
    assert(mem[1].start + mem[1].len <= mem[2].start);
    assert(mem[2].start + mem[2].len <= mem[3].start);
    assert(mem[3].start + mem[3].len <= mem[4].start);
    assert(mem[4].start + mem[4].len <= 0x100000000);
}
