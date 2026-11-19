// art.h - original procedural pixel-art: 16x16 block textures and item icons.
// Everything is generated in code (no image files, nothing copied from other games).
#pragma once
#include "game.h"

namespace vx {
using Px = uint32_t;  // bytes in memory: R,G,B,A (little endian) = raylib PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
inline int cl(int v) { return std::max(0, std::min(255, v)); }
inline Px rgba(int r, int g, int b, int a = 255) { return (uint32_t)cl(r) | ((uint32_t)cl(g) << 8) | ((uint32_t)cl(b) << 16) | ((uint32_t)cl(a) << 24); }
inline int pr(Px p) { return p & 255; }
inline int pg(Px p) { return (p >> 8) & 255; }
inline int pb(Px p) { return (p >> 16) & 255; }
inline int pa(Px p) { return (p >> 24) & 255; }
inline Px shade(Px c, int d) { return rgba(pr(c) + d, pg(c) + d, pb(c) + d, pa(c)); }

struct Tex {
  Px p[256];
  Px& at(int x, int y) { return p[y * 16 + x]; }
  void fill(Px c) { for (auto& q : p) q = c; }
};
inline Tex speckle(int r, int g, int b, int amp, uint32_t seed, int a = 255) {
  Tex t;
  for (int y = 0; y < 16; y++)
    for (int x = 0; x < 16; x++) {
      int d = (int)((hash3(seed, x, y, 1) - .5f) * 2 * amp);
      t.at(x, y) = rgba(r + d, g + d, b + d, a);
    }
  return t;
}
inline Tex oreTile(Px oc, uint32_t sd) {
  Tex t = speckle(125, 125, 125, 10, 3);
  for (int k = 0; k < 6; k++) {
    int cx = 2 + (int)(hash3(sd, k, 1, 2) * 12), cy = 2 + (int)(hash3(sd, k, 2, 3) * 12);
    for (int dy = 0; dy < 2; dy++)
      for (int dx = 0; dx < 2; dx++)
        if (hash3(sd, k * 7 + dx, dy, 5) > .2f) t.at(cx + dx, cy + dy) = shade(oc, (int)((hash3(sd, cx + dx, cy + dy, 9) - .5f) * 30));
  }
  return t;
}
inline Tex makeTile(int id) {
  Tex t;
  switch (id) {
    case T_GRASS_T: t = speckle(88, 160, 58, 14, 11); break;
    case T_GRASS_S:
      t = speckle(121, 85, 58, 12, 12);
      for (int x = 0; x < 16; x++) {
        int d = 3 + (int)(hash3(13, x, 0, 0) * 3);
        for (int y = 0; y < d; y++) t.at(x, y) = shade(rgba(88, 160, 58), (int)((hash3(14, x, y, 0) - .5f) * 24));
      }
      break;
    case T_DIRT:
      t = speckle(121, 85, 58, 14, 15);
      for (int i = 0; i < 10; i++) t.at((int)(hash3(16, i, 0, 0) * 16), (int)(hash3(16, i, 1, 0) * 16)) = rgba(92, 62, 40);
      break;
    case T_STONE:
      t = speckle(125, 125, 125, 12, 17);
      for (int i = 0; i < 5; i++) {
        int cx = (int)(hash3(18, i, 0, 0) * 14), cy = (int)(hash3(18, i, 1, 0) * 14);
        t.at(cx, cy) = t.at(cx + 1, cy) = t.at(cx, cy + 1) = rgba(98, 98, 98);
      }
      break;
    case T_COBBLE:
      t = speckle(112, 114, 114, 14, 19);
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++)
          if ((x + 3 * (y / 5)) % 5 == 0 || y % 5 == 0) t.at(x, y) = shade(t.at(x, y), -38);
      break;
    case T_SAND: t = speckle(219, 210, 155, 8, 20); break;
    case T_GRAVEL: t = speckle(133, 127, 123, 30, 21); break;
    case T_LOG_S:
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) t.at(x, y) = rgba(110 + (int)((hash3(22, x, 0, 0) - .5f) * 40), 75 + (int)((hash3(22, x, 0, 0) - .5f) * 28), 42, 255);
      break;
    case T_LOG_T:
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
          float d = std::max(std::fabs(x - 7.5f), std::fabs(y - 7.5f));
          t.at(x, y) = d > 6.5f ? rgba(100, 68, 38) : ((int)d % 2 ? rgba(176, 138, 82) : rgba(150, 112, 64));
        }
      break;
    case T_LEAVES:
      t = speckle(55, 125, 45, 28, 23);
      for (int i = 0; i < 40; i++) { int x = (int)(hash3(24, i, 0, 0) * 16), y = (int)(hash3(24, i, 1, 0) * 16); t.at(x, y) = shade(t.at(x, y), -45); }
      break;
    case T_PLANKS:
      t = speckle(190, 145, 90, 8, 25);
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++)
          if (y % 4 == 3 || x == ((y / 4) % 2 ? 3 : 11)) t.at(x, y) = shade(t.at(x, y), -28);
      break;
    case T_GLASS:
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
          bool edge = x == 0 || y == 0 || x == 15 || y == 15;
          t.at(x, y) = edge ? rgba(200, 235, 245, 235) : (x == y && x < 7 && x > 1) ? rgba(255, 255, 255, 170) : rgba(190, 225, 240, 55);
        }
      break;
    case T_BRICKS:
      t = speckle(170, 85, 70, 10, 26);
      for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++)
          if (y % 4 == 3 || (x + ((y / 4) % 2) * 4) % 8 == 0) t.at(x, y) = rgba(200, 196, 190);
      break;
    case T_COAL: t = oreTile(rgba(30, 30, 32), 31); break;
    case T_IRON: t = oreTile(rgba(214, 175, 140), 32); break;
    case T_GOLD: t = oreTile(rgba(250, 214, 60), 33); break;
    case T_DIAMOND: t = oreTile(rgba(80, 230, 230), 34); break;
    case T_BEDROCK: t = speckle(50, 50, 50, 35, 35); break;
    case T_SNOW: t = speckle(240, 246, 255, 6, 36); break;
    case T_ICE: t = speckle(166, 212, 245, 10, 37, 200); break;
    case T_WATER: t = speckle(47, 111, 214, 14, 38, 160); break;
    case T_LAVA:
      t = speckle(255, 106, 26, 40, 39);
      for (int i = 0; i < 20; i++) t.at((int)(hash3(40, i, 0, 0) * 16), (int)(hash3(40, i, 1, 0) * 16)) = rgba(255, 220, 90);
      break;
    case T_SANDSTONE:
      t = speckle(216, 200, 150, 8, 41);
      for (int y = 0; y < 16; y += 5) for (int x = 0; x < 16; x++) t.at(x, y) = shade(t.at(x, y), -16);
      break;
    case T_TABLE_T:
      t = makeTile(T_PLANKS);
      for (int i = 0; i < 16; i++) { t.at(5, i) = t.at(10, i) = t.at(i, 5) = t.at(i, 10) = rgba(110, 80, 45); }
      break;
    case T_TABLE_S:
      t = makeTile(T_PLANKS);
      for (int i = 0; i < 16; i++) { t.at(i, 0) = t.at(i, 15) = t.at(0, i) = t.at(15, i) = rgba(100, 70, 40); }
      for (int x = 3; x < 13; x++) { t.at(x, 5) = rgba(150, 150, 155); t.at(x, 9) = rgba(120, 90, 55); }
      break;
    case T_FURN_F:
      t = makeTile(T_COBBLE);
      for (int y = 6; y < 13; y++) for (int x = 4; x < 12; x++) t.at(x, y) = rgba(35, 35, 38);
      for (int x = 5; x < 11; x++) t.at(x, 12) = rgba(255, 140, 40);
      break;
    case T_NETHER: t = speckle(122, 38, 38, 25, 42); break;
    case T_SOUL:
      t = speckle(84, 64, 47, 12, 43);
      for (int i = 0; i < 12; i++) t.at((int)(hash3(44, i, 0, 0) * 16), (int)(hash3(44, i, 1, 0) * 16)) = rgba(50, 38, 28);
      break;
    case T_GLOW:
      t = speckle(255, 211, 107, 22, 45);
      for (int i = 0; i < 16; i++) t.at((int)(hash3(46, i, 0, 0) * 16), (int)(hash3(46, i, 1, 0) * 16)) = rgba(190, 130, 50);
      break;
    case T_END:
      t = speckle(224, 223, 174, 10, 47);
      for (int i = 0; i < 12; i++) t.at((int)(hash3(48, i, 0, 0) * 16), (int)(hash3(48, i, 1, 0) * 16)) = rgba(190, 188, 140);
      break;
    case T_OBS:
      t = speckle(29, 16, 51, 14, 49);
      for (int i = 0; i < 14; i++) t.at((int)(hash3(50, i, 0, 0) * 16), (int)(hash3(50, i, 1, 0) * 16)) = rgba(86, 52, 126);
      break;
    case T_PORTAL:
      t = speckle(130, 50, 210, 40, 52, 175);
      for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) if (((x + y * 2) % 7 == 0) || ((x * 3 - y) % 11 == 0)) t.at(x, y) = rgba(200, 140, 255, 205);
      break;
    case T_TORCH:
      t = speckle(112, 78, 42, 14, 60);
      for (int y = 0; y < 5; y++) for (int x = 0; x < 16; x++) t.at(x, y) = (y < 2) ? rgba(255, 240, 150) : rgba(255, 170, 40);
      break;
    case T_EMERALD: t = oreTile(rgba(40, 210, 110), 61); break;
    case T_CHEST_T:
      t = makeTile(T_PLANKS);
      for (int i = 0; i < 16; i++) { t.at(i, 0) = t.at(i, 15) = t.at(0, i) = t.at(15, i) = rgba(80, 52, 28); }
      break;
    case T_CHEST_S:
      t = makeTile(T_PLANKS);
      for (int i = 0; i < 16; i++) { t.at(i, 0) = t.at(i, 15) = t.at(0, i) = t.at(15, i) = rgba(80, 52, 28); t.at(i, 6) = rgba(70, 46, 24); }
      for (int y = 5; y < 9; y++) for (int x = 7; x < 9; x++) t.at(x, y) = rgba(230, 200, 70);
      break;
    case T_WIRE: t = speckle(70, 30, 30, 10, 62); for (int i = 0; i < 16; i++) t.at(i, 7) = t.at(i, 8) = rgba(120, 30, 30); break;
    case T_WIRE_ON: t = speckle(230, 40, 40, 14, 63); for (int i = 0; i < 16; i++) t.at(i, 7) = t.at(i, 8) = rgba(255, 150, 120); break;
    case T_LEVER: t = speckle(120, 85, 50, 10, 64); for (int y = 0; y < 5; y++) for (int x = 0; x < 16; x++) t.at(x, y) = rgba(150, 150, 155); break;
    case T_LEVER_ON: t = speckle(120, 85, 50, 10, 65); for (int y = 0; y < 5; y++) for (int x = 0; x < 16; x++) t.at(x, y) = rgba(230, 60, 50); break;
    case T_BUTTON: t = speckle(130, 130, 134, 10, 66); for (int i = 0; i < 16; i++) t.at(i, 0) = rgba(175, 175, 180); break;
    case T_BUTTON_ON: t = speckle(95, 95, 100, 8, 67); break;
    case T_PLATE: t = speckle(150, 150, 154, 8, 68); for (int i = 0; i < 16; i++) t.at(i, 0) = t.at(0, i) = rgba(185, 185, 190); break;
    case T_PLATE_ON: t = speckle(105, 105, 110, 8, 69); break;
    case T_LAMP: t = speckle(110, 80, 50, 10, 70); for (int y = 3; y < 13; y++) for (int x = 3; x < 13; x++) t.at(x, y) = rgba(150, 120, 80); break;
    case T_LAMP_ON: t = speckle(255, 235, 150, 12, 71); for (int y = 3; y < 13; y++) for (int x = 3; x < 13; x++) t.at(x, y) = rgba(255, 255, 215); break;
    case T_DOOR: t = makeTile(T_PLANKS); for (int i = 0; i < 16; i++) { t.at(i, 0) = t.at(i, 15) = t.at(0, i) = t.at(15, i) = rgba(110, 76, 42); } for (int y = 3; y < 13; y++) { t.at(4, y) = t.at(11, y) = rgba(120, 84, 48); } t.at(12, 8) = t.at(12, 9) = rgba(235, 215, 90); break;
    case T_DOOR_OPEN: t = makeTile(T_DOOR); for (auto& q : t.p) q = shade(q, 18); break;
    case T_FRAME_T: t = speckle(40, 70, 60, 12, 72); for (int y = 5; y < 11; y++) for (int x = 5; x < 11; x++) t.at(x, y) = rgba(20, 28, 26); break;
    case T_FRAME_S: t = speckle(55, 90, 78, 12, 73); for (int i = 0; i < 16; i++) t.at(i, 0) = rgba(90, 140, 110); break;
    case T_FRAME_EYE_T: t = makeTile(T_FRAME_T); for (int y = 5; y < 11; y++) for (int x = 5; x < 11; x++) t.at(x, y) = (x >= 7 && x <= 8 && y >= 7 && y <= 8) ? rgba(20, 20, 20) : rgba(60, 220, 140); break;
    case T_ENDPORTAL: t = speckle(12, 8, 30, 8, 74); for (int i = 0; i < 18; i++) t.at((int)(hash3(75, i, 0, 0) * 16), (int)(hash3(75, i, 1, 0) * 16)) = rgba(120, 220, 200); break;
    default: t = speckle(255, 0, 255, 0, 1); break;
  }
  return t;
}
// Saturation boost used for the "Vibrant Visuals" atlas.
inline Tex saturated(Tex t, float s) {
  for (auto& p : t.p) {
    float lum = .3f * pr(p) + .59f * pg(p) + .11f * pb(p);
    p = rgba((int)(lum + (pr(p) - lum) * s), (int)(lum + (pg(p) - lum) * s), (int)(lum + (pb(p) - lum) * s), pa(p));
  }
  return t;
}

inline Px tierColor(int t) {
  static const Px c[5] = {rgba(168, 128, 74), rgba(135, 135, 138), rgba(220, 220, 226), rgba(250, 215, 60), rgba(80, 230, 230)};
  return c[t];
}
inline Px armorColor(int id) {
  static const Px mc[4] = {rgba(160, 100, 60), rgba(220, 220, 226), rgba(250, 215, 60), rgba(80, 230, 230)};
  return mc[armorMat(id)];
}
inline Tex makeIcon(int id) {
  Tex t;
  t.fill(0);
  auto P = [&](int x, int y, Px c) { if (x >= 0 && x < 16 && y >= 0 && y < 16) t.at(x, y) = c; };
  const Px brown = rgba(112, 78, 42), darkb = rgba(84, 58, 30);
  if (id > 0 && id < BLK_COUNT) {  // isometric cube from the block's own textures
    const BInfo& b = binfo(id);
    Tex top = makeTile(b.top), side = makeTile(b.side);
    for (int y = 0; y < 16; y++)
      for (int x = 0; x < 16; x++) {
        float dx = x - 7.5f;
        if (std::fabs(dx) / 7.5f + std::fabs(y - 3.75f) / 3.75f <= 1.f) { t.at(x, y) = top.at(x, std::min(15, y * 2)); continue; }
        float yt = x < 8 ? 3.75f + x * .5f : 7.5f - (x - 7.5f) * .5f + 0.f, yb = x < 8 ? 11.25f + x * .5f : 15 - (x - 7.5f) * .5f;
        if (y >= yt && y <= yb) {
          Px c = side.at(x < 8 ? x * 2 : (x - 8) * 2, std::max(0, std::min(15, (int)((y - yt) * 1.4f))));
          t.at(x, y) = shade(c, x < 8 ? -28 : -55);
        }
      }
    return t;
  }
  if (isArmor(id)) {
    int pc = armorPiece(id);
    Px c = armorColor(id), dk = shade(c, -40), lt = shade(c, 35);
    auto R = [&](int x0, int y0, int x1, int y1) { for (int y = y0; y <= y1; y++) for (int x = x0; x <= x1; x++) P(x, y, (y == y0 || x == x0) ? lt : (y == y1 || x == x1) ? dk : c); };
    if (pc == 0) { R(3, 3, 12, 8); R(3, 8, 5, 12); R(10, 8, 12, 12); }
    else if (pc == 1) { R(2, 2, 13, 13); for (int y = 2; y < 6; y++) for (int x = 6; x <= 9; x++) P(x, y, 0); }
    else if (pc == 2) { R(3, 2, 12, 6); R(3, 7, 6, 14); R(9, 7, 12, 14); }
    else { R(2, 8, 6, 13); R(9, 8, 13, 13); }
    return t;
  }
  if (isTool(id)) {
    int ty = toolType(id), tr = toolTier(id);
    Px col = tierColor(tr), dk = shade(col, -35), lt = shade(col, 30);
    if (ty == 3) {  // sword
      for (int i = 0; i < 9; i++) { P(5 + i, 11 - i, col); P(6 + i, 11 - i, lt); P(5 + i, 12 - i, dk); }
      for (int i = 0; i < 4; i++) P(3 + i, 9 + i, darkb);
      P(2, 14, brown); P(3, 13, brown); P(4, 12, brown);
      return t;
    }
    for (int i = 0; i < 10; i++) { P(3 + i, 13 - i, brown); P(4 + i, 13 - i, darkb); }
    if (ty == 0) { for (int x = 4; x <= 13; x++) { P(x, 3, col); P(x, 4, dk); } P(4, 5, col); P(13, 5, col); P(3, 6, dk); P(14, 6, dk); }
    if (ty == 1) { for (int dy = 2; dy <= 7; dy++) for (int dx = 8; dx <= 12; dx++) P(dx, dy, dy < 4 ? lt : col); for (int dy = 3; dy <= 6; dy++) P(7, dy, dk); P(13, 4, dk); P(13, 5, dk); }
    if (ty == 2) { for (int dy = 1; dy <= 5; dy++) for (int dx = 10; dx <= 14; dx++) if (!((dx == 10 || dx == 14) && (dy == 1 || dy == 5))) P(dx, dy, dx == 10 ? lt : col); }
    if (ty == 4) { for (int x = 8; x <= 13; x++) { P(x, 3, col); P(x, 2, dk); } P(8, 4, col); P(8, 5, dk); }
    return t;
  }
  switch (id) {
    case I_STICK: for (int i = 0; i < 11; i++) { P(3 + i, 13 - i, brown); P(4 + i, 13 - i, darkb); } break;
    case I_COAL:
      for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { float d = std::hypot(x - 7.5f, y - 8.f); if (d < 4.6f + hash3(60, x, y, 0) * .8f) t.at(x, y) = rgba(34 + (int)(hash3(61, x, y, 0) * 26), 34 + (int)(hash3(61, x, y, 0) * 26), 38 + (int)(hash3(61, x, y, 0) * 26)); }
      break;
    case I_IRON: case I_GOLD: {
      Px c = id == I_IRON ? rgba(222, 222, 228) : rgba(250, 214, 60);
      for (int y = 5; y <= 10; y++) for (int x = 3 + (10 - y); x <= 11 + (10 - y); x++) P(x, y, y == 5 ? shade(c, 35) : y == 10 ? shade(c, -45) : c);
      break;
    }
    case I_DIAMOND:
      for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { int d = (int)(std::fabs(x - 7.5f) + std::fabs(y - 7.5f)); if (d <= 6) t.at(x, y) = d >= 6 ? rgba(40, 150, 170) : (x < 8 && y < 8) ? rgba(190, 255, 255) : rgba(80, 225, 230); }
      break;
    case I_APPLE:
      for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) if (std::hypot(x - 7.5f, y - 9.f) < 5.2f) t.at(x, y) = (x < 7 && y < 9) ? rgba(240, 90, 80) : rgba(206, 46, 40);
      P(8, 3, brown); P(8, 4, brown); P(9, 3, rgba(70, 160, 60)); P(10, 3, rgba(70, 160, 60)); P(9, 4, rgba(70, 160, 60));
      break;
    case I_RAW_BEEF: case I_STEAK: case I_RAW_PORK: case I_PORK: case I_RAW_CHICKEN: case I_CHICKEN: {
      Px c = id == I_RAW_BEEF ? rgba(200, 70, 70) : id == I_STEAK ? rgba(130, 70, 40) : id == I_RAW_PORK ? rgba(240, 150, 150) : id == I_PORK ? rgba(190, 110, 70) : id == I_RAW_CHICKEN ? rgba(240, 200, 190) : rgba(200, 150, 80);
      for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) if ((x - 7) * (x - 7) / 25.f + (y - 8) * (y - 8) / 16.f <= 1.f) P(x, y, (x + y < 14) ? shade(c, 28) : shade(c, -14));
      P(12, 11, rgba(235, 230, 215)); P(13, 12, rgba(235, 230, 215)); P(13, 11, rgba(235, 230, 215)); P(12, 12, rgba(235, 230, 215));
      break;
    }
    case I_LEATHER: for (int y = 4; y <= 11; y++) for (int x = 3; x <= 12; x++) P(x, y, shade(rgba(150, 95, 55), (int)((hash3(70, x, y, 0) - .5f) * 26))); break;
    case I_WOOL: for (int y = 4; y <= 11; y++) for (int x = 4; x <= 11; x++) if (!((x == 4 || x == 11) && (y == 4 || y == 11))) P(x, y, shade(rgba(238, 238, 238), -(int)(hash3(71, x, y, 0) * 22))); break;
    case I_BONE: for (int i = 0; i < 9; i++) { P(4 + i, 12 - i, rgba(235, 232, 220)); P(5 + i, 12 - i, rgba(200, 198, 188)); } P(3, 12, rgba(235, 232, 220)); P(4, 13, rgba(235, 232, 220)); P(13, 3, rgba(235, 232, 220)); P(12, 2, rgba(235, 232, 220)); break;
    case I_STRING: for (int i = 0; i < 12; i++) P(2 + i, (i % 4 < 2) ? 6 + (i % 2) : 9 + (i % 2), rgba(235, 235, 235)); break;
    case I_GUNPOWDER: for (int k = 0; k < 46; k++) P(4 + (int)(hash3(72, k, 0, 0) * 8), 6 + (int)(hash3(72, k, 1, 0) * 6), rgba(70 + (int)(hash3(72, k, 2, 0) * 60), 70 + (int)(hash3(72, k, 2, 0) * 60), 74 + (int)(hash3(72, k, 2, 0) * 60))); break;
    case I_ROTTEN: for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) if (std::hypot(x - 7.5f, y - 8.f) < 5.f + hash3(73, x, y, 0)) P(x, y, hash3(74, x, y, 0) < .25f ? rgba(70, 60, 40) : rgba(112, 135, 72)); break;
    case I_FLINT: for (int y = 4; y <= 12; y++) for (int x = 0; x < 16; x++) if (std::abs(x - 8) <= (y - 4) / 2 + 1) P(x, y, (x < 8) ? rgba(80, 80, 86) : rgba(48, 48, 52)); break;
    case I_FLINT_STEEL:
      for (int y = 9; y <= 12; y++) for (int x = 3; x <= 9; x++) P(x, y, y == 9 ? rgba(235, 235, 240) : rgba(190, 190, 198));
      for (int y = 3; y <= 8; y++) for (int x = 9; x <= 13; x++) if (x - 9 <= y - 3) P(x, y, rgba(60, 60, 66));
      break;
    case I_ARROW: for (int i = 0; i < 10; i++) P(3 + i, 13 - i, brown); P(13, 3, rgba(170, 170, 175)); P(12, 3, rgba(170, 170, 175)); P(13, 4, rgba(170, 170, 175)); P(2, 13, rgba(235, 235, 235)); P(3, 14, rgba(235, 235, 235)); P(3, 13, rgba(210, 60, 60)); break;
    case I_BOW:
      for (int y = 2; y <= 13; y++) { int x = 4 + (int)(4 * std::sin((y - 2) / 11.f * 3.14159f)); P(x, y, brown); P(x + 1, y, darkb); }
      for (int y = 3; y <= 12; y++) P(11 - (y == 3 || y == 12 ? 0 : 0), y, rgba(235, 235, 235));
      break;
    case I_BUCKET: case I_WBUCKET: case I_LBUCKET:
      for (int y = 5; y <= 13; y++) { int hw = 5 - (y - 5) / 3; for (int x = 8 - hw; x < 8 + hw; x++) P(x, y, y == 5 ? rgba(215, 215, 222) : (x < 8 ? rgba(180, 180, 188) : rgba(140, 140, 150))); }
      if (id != I_BUCKET) for (int y = 6; y <= 7; y++) for (int x = 4; x < 12; x++) P(x, y, id == I_WBUCKET ? rgba(60, 120, 230) : rgba(255, 130, 30));
      break;
    case I_FEATHER: for (int i = 0; i < 10; i++) { P(4 + i, 12 - i, rgba(245, 245, 245)); P(5 + i, 12 - i, rgba(215, 215, 220)); P(4 + i, 11 - i, rgba(235, 235, 238)); } break;
    case I_EMERALD:
      for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) { int d = (int)(std::fabs(x - 7.5f) * .8f + std::fabs(y - 7.5f)); if (d <= 6) t.at(x, y) = d >= 6 ? rgba(20, 120, 60) : (x < 8 && y < 8) ? rgba(150, 255, 190) : rgba(40, 205, 105); }
      break;
    case I_PEARL: case I_EYE:
      for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) if (std::hypot(x - 7.5f, y - 7.5f) < 5.3f) P(x, y, id == I_PEARL ? ((x < 7 && y < 7) ? rgba(110, 230, 215) : rgba(30, 150, 140)) : ((x < 7 && y < 7) ? rgba(130, 255, 160) : rgba(40, 170, 90)));
      if (id == I_EYE) { P(7, 7, rgba(15, 15, 15)); P(8, 7, rgba(15, 15, 15)); P(7, 8, rgba(15, 15, 15)); P(8, 8, rgba(15, 15, 15)); }
      break;
    case I_EMBER: for (int i = 0; i < 10; i++) { P(3 + i, 13 - i, rgba(255, 150, 30)); P(4 + i, 13 - i, rgba(255, 215, 80)); P(3 + i, 12 - i, rgba(200, 90, 20)); } break;
    case I_SHIELD:
      for (int y = 1; y <= 14; y++) { int hw = y < 9 ? 5 : 5 - (y - 8); for (int x = 8 - hw; x < 8 + hw; x++) P(x, y, (x == 8 - hw || y == 1) ? rgba(235, 235, 240) : (x < 8 ? rgba(150, 100, 60) : rgba(125, 80, 45))); }
      for (int y = 3; y <= 9; y++) { P(7, y, rgba(200, 200, 208)); P(8, y, rgba(170, 170, 180)); }
      break;
    default: for (int y = 5; y < 11; y++) for (int x = 5; x < 11; x++) P(x, y, rgba(255, 0, 255)); break;
  }
  return t;
}
}  // namespace vx
