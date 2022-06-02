
namespace PixelFont {

Index::Index(
  uint32_t glyphIndex,
  uint32_t minCodePoint,
  uint32_t maxCodePoint
) : m_glyphIndex(glyphIndex),
    m_minCodePoint(minCodePoint),
    m_maxCodePoint(maxCodePoint)
{}

Index::Index(uint32_t codePoint) : m_maxCodePoint(codePoint) {}

Font::Font(
  const std::vector<Glyph>& glyphs,
  const std::vector<Index>& index,
  int height,
  int kmax
) : m_glyphs(glyphs),
    m_index(index),
    m_height(height),
    m_kmax(kmax)
{}

uint32_t Font::find_glyph(uint32_t codePoint) const {
  auto it = std::lower_bound(
    m_index.begin(),
    m_index.end(),
    codePoint,
    [](const Index& first, uint32_t value) {
      return first.m_maxCodePoint < value;
    }
  );
  if (it == m_index.end()) {
    return UINT32_MAX;
  }

  uint32_t index;
  if (codePoint < it->m_minCodePoint) {
    index = UINT32_MAX;
  } else if (codePoint > it->m_maxCodePoint) {
    index = UINT32_MAX;
  } else {
    index = it->m_glyphIndex + (codePoint - it->m_minCodePoint);
  }
  //printf("%04x = %04x + (%04x - %04x)\n", index, it->m_glyphIndex, codePoint, it->m_minCodePoint);
  return index;
}

int Font::calc_width(uint8_t* s, uint16_t len) const {
  int strwidth = 0;
  uint32_t codepoint = 0;
  uint32_t state = 0;

  // determine text width:
  for (unsigned n = 0; n < len; ++s, ++n) {
    uint8_t c = *s;
    if (c == 0) break;

    if (decode(&state, &codepoint, c)) {
      continue;
    }

    auto glyphIndex = find_glyph(codepoint);
    if (glyphIndex == UINT32_MAX) {
      continue;
    }

    const auto& g = m_glyphs[glyphIndex];
    strwidth += g.m_width;
  }

  return strwidth;
}

}
