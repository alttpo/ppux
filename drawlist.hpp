#pragma once

#include <cstring>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <vector>
#include <stdexcept>
#include <utility>

#include "pixelfont.hpp"
#include "drawlist_fwd.hpp"

namespace DrawList {

struct FontContainer {
  void load_pcf(int fontindex, const uint8_t* pcf_data, int pcf_size);

  void clear();
  void erase(int fontindex);
  int size() const;

  std::shared_ptr<PixelFont::Font> operator[](int fontindex) const;

private:
  std::vector<std::shared_ptr<PixelFont::Font>> m_fonts;
};

struct Space {
  virtual ~Space() = 0;

  virtual uint8_t* vram_data() = 0;
  virtual uint32_t vram_size() const = 0;

  virtual uint8_t* cgram_data() = 0;
  virtual uint32_t cgram_size() const = 0;
};

struct LocalSpace : public Space {
  LocalSpace(uint8_t* vram, uint8_t* cgram);

  uint8_t* vram_data() final;
  uint32_t vram_size() const final;

  uint8_t* cgram_data() final;
  uint32_t cgram_size() const final;

private:
  uint8_t* m_vram;
  uint8_t* m_cgram;
};

struct ExtraSpace : public Space {
  ExtraSpace();

  uint8_t* vram_data() final;
  uint32_t vram_size() const final;

  uint8_t* cgram_data() final;
  uint32_t cgram_size() const final;

private:
  uint8_t vram[0x10000];
  uint8_t cgram[0x200];
};

typedef std::function<std::shared_ptr<Space>(int)> AllocateExtra;

struct SpaceContainer {
  static const int MaxCount = 256;

  explicit SpaceContainer(std::shared_ptr<Space> localSpace, AllocateExtra allocateExtra);

  void reset();

  std::shared_ptr<Space> operator[](int index);

  uint8_t* get_vram_space(int index);
  uint8_t* get_cgram_space(int index);

private:
  std::shared_ptr<Space> m_localSpace;
  std::vector<std::shared_ptr<Space>> m_spaces;
  AllocateExtra m_allocate;
};

struct Renderer {
  virtual void draw_pixel(int x, int y) = 0;
  virtual void draw_hline(int x, int y, int w) = 0;
  virtual void draw_vline(int x, int y, int h) = 0;
  virtual void draw_rect(int x0, int y0, int w, int h) = 0;
  virtual void draw_rect_fill(int x0, int y0, int w, int h) = 0;
  virtual void draw_line(int x1, int y1, int x2, int y2) = 0;
  virtual void draw_text_utf8(uint8_t* s, uint16_t len, PixelFont::Font& font, int x0, int y0, text_alignment align) = 0;

  virtual uint16_t* draw_image(int x0, int y0, int w, int h, uint16_t* d) = 0;
  virtual void draw_vram_tile(int x0, int y0, int w, int h, bool hflip, bool vflip, uint8_t bpp, uint16_t vram_addr, uint8_t palette, uint8_t* vram, uint8_t* cgram) = 0;
};

struct State {
  State();

  draw_layer layer;
  uint8_t priority;

  uint16_t stroke_color, outline_color, fill_color;

  int xOffset, yOffset;
};

typedef std::function<void(State& state, std::shared_ptr<Renderer>& o_target)> ChooseRenderer;
typedef std::function<void(draw_layer layer, int& xOffset, int& yOffset)> GetBGOffsets;

struct Context {
  Context(
    const ChooseRenderer& chooseRenderer,
    const GetBGOffsets& getBGOffsets,
    std::shared_ptr<FontContainer>   fonts,
    std::shared_ptr<SpaceContainer>  spaces
  );

  void draw_list(const std::vector<uint16_t>& cmdlist);

  State state;

private:
  std::shared_ptr<Renderer> m_renderer;

  const ChooseRenderer& m_chooseRenderer;
  const GetBGOffsets& m_getBGOffsets;

  const std::shared_ptr<FontContainer> m_fonts;
  const std::shared_ptr<SpaceContainer> m_spaces;
};

}
