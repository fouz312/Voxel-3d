// game.h - Voxel3D game logic. No raylib dependency, so it can be unit tested on any machine.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <string>
#include <unordered_map>
#include <vector>

namespace vx {
constexpr int CS = 16, WH = 64, SEA = 18;
constexpr float DAYLEN = 600.f;
enum Mode { SURVIVAL = 0, CREATIVE = 1, HARDCORE = 2 };
enum Dim { OVERWORLD = 0, NETHER = 1, END = 2 };

enum Blk : uint8_t {
  AIR, GRASS, DIRT, STONE, COBBLE, SAND, GRAVEL, LOG, LEAVES, PLANKS, GLASS, BRICKS,
  COAL_ORE, IRON_ORE, GOLD_ORE, DIAMOND_ORE, BEDROCK, SNOW, ICE, WATER, LAVA, SANDSTONE,
  TABLE, FURNACE, NETHERRACK, SOULSAND, GLOWSTONE, ENDSTONE, OBSIDIAN, PORTAL,
  TORCH, EMERALD_ORE, CHEST, WIRE, WIRE_ON, LEVER, LEVER_ON, BUTTON, BUTTON_ON, PLATE, PLATE_ON, LAMP, LAMP_ON, DOOR, DOOR_OPEN,
  END_FRAME, END_FRAME_EYE, END_PORTAL, BLK_COUNT
};
enum Item {
  I_STICK = 100, I_COAL, I_IRON, I_GOLD, I_DIAMOND, I_APPLE, I_RAW_BEEF, I_STEAK, I_RAW_PORK, I_PORK, I_RAW_CHICKEN, I_CHICKEN,
  I_LEATHER, I_WOOL, I_BONE, I_STRING, I_GUNPOWDER, I_ROTTEN, I_FLINT, I_FLINT_STEEL,
  I_TOOL0 = 120,   // 25 tools: 120 + type*5 + tier
  I_ARMOR0 = 150,  // 16 armor pieces: 150 + material*4 + piece
  I_ARROW = 170, I_BOW, I_BUCKET, I_WBUCKET, I_LBUCKET, I_FEATHER, I_EMERALD, I_PEARL, I_EMBER, I_EYE, I_SHIELD, I_END = 190
};
enum Tile {
  T_GRASS_T, T_GRASS_S, T_DIRT, T_STONE, T_COBBLE, T_SAND, T_GRAVEL, T_LOG_S, T_LOG_T, T_LEAVES, T_PLANKS,
  T_GLASS, T_BRICKS, T_COAL, T_IRON, T_GOLD, T_DIAMOND, T_BEDROCK, T_SNOW, T_ICE, T_WATER, T_LAVA,
  T_SANDSTONE, T_TABLE_T, T_TABLE_S, T_FURN_F, T_NETHER, T_SOUL, T_GLOW, T_END, T_OBS, T_PORTAL,
  T_TORCH, T_EMERALD, T_CHEST_T, T_CHEST_S, T_WIRE, T_WIRE_ON, T_LEVER, T_LEVER_ON, T_BUTTON, T_BUTTON_ON, T_PLATE, T_PLATE_ON,
  T_LAMP, T_LAMP_ON, T_DOOR, T_DOOR_OPEN, T_FRAME_T, T_FRAME_S, T_FRAME_EYE_T, T_ENDPORTAL, T_COUNT
};

struct BInfo {
  const char* name;
  float hard;
  int tool;  // preferred tool: 0 pickaxe, 1 axe, 2 shovel, -1 none
  int req;   // harvest level required (0 = none)
  int drop;  // -1 = itself, -2 = nothing, else item id
  bool solid, trans;
  int top, side, bot;
};
inline const BInfo& binfo(int b) {
  static const BInfo T[BLK_COUNT] = {
      {"Air", 0, -1, 0, -2, false, true, 0, 0, 0},
      {"Grass Block", .6f, 2, 0, DIRT, true, false, T_GRASS_T, T_GRASS_S, T_DIRT},
      {"Dirt", .5f, 2, 0, -1, true, false, T_DIRT, T_DIRT, T_DIRT},
      {"Stone", 1.5f, 0, 1, COBBLE, true, false, T_STONE, T_STONE, T_STONE},
      {"Cobblestone", 2, 0, 1, -1, true, false, T_COBBLE, T_COBBLE, T_COBBLE},
      {"Sand", .5f, 2, 0, -1, true, false, T_SAND, T_SAND, T_SAND},
      {"Gravel", .6f, 2, 0, -1, true, false, T_GRAVEL, T_GRAVEL, T_GRAVEL},
      {"Log", 2, 1, 0, -1, true, false, T_LOG_T, T_LOG_S, T_LOG_T},
      {"Leaves", .2f, -1, 0, -2, true, false, T_LEAVES, T_LEAVES, T_LEAVES},
      {"Planks", 2, 1, 0, -1, true, false, T_PLANKS, T_PLANKS, T_PLANKS},
      {"Glass", .3f, -1, 0, -2, true, true, T_GLASS, T_GLASS, T_GLASS},
      {"Bricks", 2, 0, 1, -1, true, false, T_BRICKS, T_BRICKS, T_BRICKS},
      {"Coal Ore", 3, 0, 1, I_COAL, true, false, T_COAL, T_COAL, T_COAL},
      {"Iron Ore", 3, 0, 2, -1, true, false, T_IRON, T_IRON, T_IRON},
      {"Gold Ore", 3, 0, 3, -1, true, false, T_GOLD, T_GOLD, T_GOLD},
      {"Diamond Ore", 3, 0, 3, I_DIAMOND, true, false, T_DIAMOND, T_DIAMOND, T_DIAMOND},
      {"Bedrock", 1e9f, -1, 99, -2, true, false, T_BEDROCK, T_BEDROCK, T_BEDROCK},
      {"Snow", .2f, 2, 0, -1, true, false, T_SNOW, T_SNOW, T_SNOW},
      {"Ice", .5f, 0, 0, -2, true, true, T_ICE, T_ICE, T_ICE},
      {"Water", 0, -1, 0, -2, false, true, T_WATER, T_WATER, T_WATER},
      {"Lava", 0, -1, 0, -2, false, true, T_LAVA, T_LAVA, T_LAVA},
      {"Sandstone", .8f, 0, 1, -1, true, false, T_SANDSTONE, T_SANDSTONE, T_SANDSTONE},
      {"Crafting Table", 2.5f, 1, 0, -1, true, false, T_TABLE_T, T_TABLE_S, T_PLANKS},
      {"Furnace", 3.5f, 0, 1, -1, true, false, T_COBBLE, T_FURN_F, T_COBBLE},
      {"Netherrack", .4f, 0, 1, -1, true, false, T_NETHER, T_NETHER, T_NETHER},
      {"Soul Sand", .5f, 2, 0, -1, true, false, T_SOUL, T_SOUL, T_SOUL},
      {"Glowstone", .3f, -1, 0, -1, true, false, T_GLOW, T_GLOW, T_GLOW},
      {"End Stone", 3, 0, 1, -1, true, false, T_END, T_END, T_END},
      {"Obsidian", 50, 0, 4, -1, true, false, T_OBS, T_OBS, T_OBS},
      {"Nether Portal", 0, -1, 0, -2, false, true, T_PORTAL, T_PORTAL, T_PORTAL},
      {"Torch", .05f, -1, 0, -1, false, true, T_TORCH, T_TORCH, T_TORCH},
      {"Emerald Ore", 3, 0, 3, I_EMERALD, true, false, T_EMERALD, T_EMERALD, T_EMERALD},
      {"Chest", 2.5f, 1, 0, -1, true, false, T_CHEST_T, T_CHEST_S, T_CHEST_T},
      {"Circuit Wire", .05f, -1, 0, WIRE, false, true, T_WIRE, T_WIRE, T_WIRE},
      {"Circuit Wire (live)", .05f, -1, 0, WIRE, false, true, T_WIRE_ON, T_WIRE_ON, T_WIRE_ON},
      {"Lever", .5f, -1, 0, LEVER, false, true, T_LEVER, T_LEVER, T_LEVER},
      {"Lever (on)", .5f, -1, 0, LEVER, false, true, T_LEVER_ON, T_LEVER_ON, T_LEVER_ON},
      {"Button", .5f, -1, 0, BUTTON, false, true, T_BUTTON, T_BUTTON, T_BUTTON},
      {"Button (pressed)", .5f, -1, 0, BUTTON, false, true, T_BUTTON_ON, T_BUTTON_ON, T_BUTTON_ON},
      {"Pressure Plate", .5f, 0, 0, PLATE, false, true, T_PLATE, T_PLATE, T_PLATE},
      {"Pressure Plate (down)", .5f, 0, 0, PLATE, false, true, T_PLATE_ON, T_PLATE_ON, T_PLATE_ON},
      {"Lamp", .3f, -1, 0, LAMP, true, false, T_LAMP, T_LAMP, T_LAMP},
      {"Lamp (lit)", .3f, -1, 0, LAMP, true, false, T_LAMP_ON, T_LAMP_ON, T_LAMP_ON},
      {"Door", 3, 1, 0, DOOR, true, false, T_DOOR, T_DOOR, T_DOOR},
      {"Door (open)", 3, 1, 0, DOOR, false, true, T_DOOR_OPEN, T_DOOR_OPEN, T_DOOR_OPEN},
      {"Portal Frame", 1e9f, -1, 99, -2, true, false, T_FRAME_T, T_FRAME_S, T_FRAME_S},
      {"Portal Frame (eye)", 1e9f, -1, 99, -2, true, false, T_FRAME_EYE_T, T_FRAME_S, T_FRAME_S},
      {"End Portal", 0, -1, 0, -2, false, true, T_ENDPORTAL, T_ENDPORTAL, T_ENDPORTAL},
  };
  return T[b >= 0 && b < BLK_COUNT ? b : 0];
}
inline bool glows(int b) { return b == GLOWSTONE || b == LAVA || b == PORTAL || b == TORCH || b == LAMP_ON || b == END_PORTAL; }
// Blocks that are smaller than a full cube (walk-through, but can be aimed at / mined / used).
inline bool isPartial(int b) { return b == TORCH || b == WIRE || b == WIRE_ON || b == LEVER || b == LEVER_ON || b == BUTTON || b == BUTTON_ON || b == PLATE || b == PLATE_ON || b == DOOR_OPEN; }
inline bool needsSupport(int b) { return b == TORCH || b == WIRE || b == WIRE_ON || b == LEVER || b == LEVER_ON || b == BUTTON || b == BUTTON_ON || b == PLATE || b == PLATE_ON; }
inline void blockShape(int b, float& w, float& h) {  // width (x and z) and height of the drawn box
  w = 1; h = 1;
  switch (b) {
    case TORCH: w = .2f; h = .7f; break;
    case WIRE: case WIRE_ON: h = .08f; break;
    case PLATE: case PLATE_ON: w = .8f; h = .1f; break;
    case LEVER: case LEVER_ON: w = .3f; h = .6f; break;
    case BUTTON: case BUTTON_ON: w = .3f; h = .15f; break;
    case DOOR_OPEN: w = .2f; break;
    default: break;
  }
}
inline bool isCircuit(int b) { return b == WIRE || b == WIRE_ON || b == LEVER || b == LEVER_ON || b == BUTTON || b == BUTTON_ON || b == PLATE || b == PLATE_ON || b == LAMP || b == LAMP_ON || b == DOOR || b == DOOR_OPEN; }
inline bool circuitSource(int b) { return b == LEVER_ON || b == BUTTON_ON || b == PLATE_ON; }

// ---------- items & tools ----------
constexpr float TSPEED[5] = {2, 4, 6, 12, 8};
constexpr int TDUR[5] = {59, 131, 250, 32, 1561};
constexpr int TLEVEL[5] = {1, 2, 3, 1, 4};  // wood, stone, iron, gold, diamond
inline bool isTool(int id) { return id >= I_TOOL0 && id < I_TOOL0 + 25; }
inline int toolType(int id) { return (id - I_TOOL0) / 5; }  // 0 pick 1 axe 2 shovel 3 sword 4 hoe
inline int toolTier(int id) { return (id - I_TOOL0) % 5; }
inline bool isArmor(int id) { return id >= I_ARMOR0 && id < I_ARMOR0 + 16; }
inline int armorPiece(int id) { return (id - I_ARMOR0) % 4; }  // 0 helmet 1 chestplate 2 leggings 3 boots
inline int armorMat(int id) { return (id - I_ARMOR0) / 4; }    // 0 leather 1 iron 2 gold 3 diamond
constexpr int ARMOR_DEF[4][4] = {{1, 3, 2, 1}, {2, 6, 5, 2}, {2, 5, 3, 1}, {3, 8, 6, 3}};
inline int maxDur(int id) {
  if (isTool(id)) return TDUR[toolTier(id)];
  if (isArmor(id)) { static const int base[4] = {11, 16, 15, 13}, mult[4] = {5, 15, 7, 33}; return base[armorPiece(id)] * mult[armorMat(id)]; }
  if (id == I_BOW) return 384;
  if (id == I_FLINT_STEEL) return 64;
  if (id == I_SHIELD) return 336;
  return 0;
}
inline int maxStack(int id) { return (maxDur(id) > 0 || id == I_BUCKET || id == I_WBUCKET || id == I_LBUCKET) ? 1 : 64; }
inline bool validItem(int id) { return (id > 0 && id < BLK_COUNT) || (id >= I_STICK && id <= I_FLINT_STEEL) || isTool(id) || isArmor(id) || (id >= I_ARROW && id <= I_SHIELD); }
inline int toolDamage(int id) {
  static const int sw[5] = {4, 5, 6, 4, 7}, ax[5] = {3, 4, 5, 3, 6}, pk[5] = {2, 3, 4, 2, 5};
  if (!isTool(id)) return 1;
  int t = toolType(id), r = toolTier(id);
  return t == 3 ? sw[r] : t == 1 ? ax[r] : t == 4 ? 1 : pk[r];
}
inline int foodValue(int id) {
  switch (id) {
    case I_APPLE: return 4; case I_RAW_BEEF: return 3; case I_STEAK: return 8; case I_RAW_PORK: return 3; case I_PORK: return 8;
    case I_RAW_CHICKEN: return 2; case I_CHICKEN: return 6; case I_ROTTEN: return 2; default: return 0;
  }
}
inline const std::string& iname(int id) {
  static std::vector<std::string> n;
  if (n.empty()) {
    n.assign(I_END, "?");
    n[0] = "Air";
    for (int b = 1; b < BLK_COUNT; b++) n[b] = binfo(b).name;
    n[I_STICK] = "Stick"; n[I_COAL] = "Coal"; n[I_IRON] = "Iron Ingot"; n[I_GOLD] = "Gold Ingot";
    n[I_DIAMOND] = "Diamond"; n[I_APPLE] = "Apple"; n[I_RAW_BEEF] = "Raw Beef"; n[I_STEAK] = "Steak"; n[I_RAW_PORK] = "Raw Porkchop";
    n[I_PORK] = "Cooked Porkchop"; n[I_RAW_CHICKEN] = "Raw Chicken"; n[I_CHICKEN] = "Cooked Chicken"; n[I_LEATHER] = "Leather"; n[I_WOOL] = "Wool";
    n[I_BONE] = "Bone"; n[I_STRING] = "String"; n[I_GUNPOWDER] = "Gunpowder"; n[I_ROTTEN] = "Rotten Flesh"; n[I_FLINT] = "Flint";
    n[I_FLINT_STEEL] = "Flint and Steel"; n[I_ARROW] = "Arrow"; n[I_BOW] = "Bow"; n[I_BUCKET] = "Bucket"; n[I_WBUCKET] = "Water Bucket";
    n[I_LBUCKET] = "Lava Bucket"; n[I_FEATHER] = "Feather"; n[I_EMERALD] = "Emerald"; n[I_PEARL] = "Warp Pearl";
    n[I_EMBER] = "Ember Rod"; n[I_EYE] = "Seeker Eye"; n[I_SHIELD] = "Shield";
    static const char *am[] = {"Leather", "Iron", "Golden", "Diamond"}, *ap[] = {"Helmet", "Chestplate", "Leggings", "Boots"};
    for (int m = 0; m < 4; m++) for (int q = 0; q < 4; q++) n[I_ARMOR0 + m * 4 + q] = std::string(am[m]) + " " + ap[q];
    static const char *tn[] = {"Wooden", "Stone", "Iron", "Golden", "Diamond"},
                      *ty[] = {"Pickaxe", "Axe", "Shovel", "Sword", "Hoe"};
    for (int t = 0; t < 5; t++)
      for (int r = 0; r < 5; r++) n[I_TOOL0 + t * 5 + r] = std::string(tn[r]) + " " + ty[t];
  }
  return n[id >= 0 && id < I_END ? id : 0];
}

// ---------- noise / terrain / biomes ----------
inline float hash3(uint32_t seed, int x, int y, int z) {
  uint32_t h = seed * 374761393u ^ (uint32_t)x * 668265263u ^ (uint32_t)y * 2246822519u ^ (uint32_t)z * 3266489917u;
  h = (h ^ (h >> 13)) * 1274126177u;
  h ^= h >> 16;
  return (h & 0xFFFFFF) / 16777216.f;
}
inline float vnoise(uint32_t s, float x, float z, float sc) {
  x /= sc; z /= sc;
  int xi = (int)std::floor(x), zi = (int)std::floor(z);
  float fx = x - xi, fz = z - zi, u = fx * fx * (3 - 2 * fx), v = fz * fz * (3 - 2 * fz);
  float a = hash3(s, xi, 77, zi), b = hash3(s, xi + 1, 77, zi), c = hash3(s, xi, 77, zi + 1), d = hash3(s, xi + 1, 77, zi + 1);
  return a + (b - a) * u + (c - a) * v + (a - b - c + d) * u * v;
}
inline int terrainH(uint32_t s, int x, int z) {
  float h = 12 + (vnoise(s, (float)x, (float)z, 80) * .55f + vnoise(s + 1, (float)x, (float)z, 32) * .3f + vnoise(s + 2, (float)x, (float)z, 11) * .15f) * 16;
  float c = vnoise(s + 5, (float)x, (float)z, 200);
  if (c < .34f) h -= (.34f - c) * 40;  // oceans
  float m = vnoise(s + 3, x + 900.f, z - 300.f, 70);
  if (m > .5f) h += (m - .5f) * 45 * (.5f + vnoise(s + 4, (float)x, (float)z, 17));  // mountains
  if (std::fabs(vnoise(s + 6, (float)x, (float)z, 60) - .5f) < .03f) h = std::min(h, (float)SEA - 2);  // rivers
  return std::max(2, std::min((int)h, WH - 8));
}
enum Biome { PLAINS, FOREST, DESERT, SNOWY, MOUNTAINS, OCEAN, BEACH };
inline const char* biomeName(int b) {
  static const char* n[] = {"Plains", "Forest", "Desert", "Snowy Plains", "Mountains", "Ocean", "Beach"};
  return n[b];
}
inline int biomeAt(uint32_t s, int x, int z, int h) {
  if (h <= SEA - 3) return OCEAN;
  if (h <= SEA + 1) return BEACH;
  if (h > 38) return MOUNTAINS;
  float t = vnoise(s + 7, (float)x, (float)z, 170), m = vnoise(s + 8, (float)x, (float)z, 110);
  if (t > .66f) return DESERT;
  if (t < .3f) return SNOWY;
  return m > .55f ? FOREST : PLAINS;
}

// ---------- world ----------
struct Chunk {
  std::array<uint8_t, CS * CS * WH> d{};
  uint32_t ver = 1;
};
inline int idx(int x, int y, int z) { return (y * CS + z) * CS + x; }
inline uint64_t ckey(int cx, int cz) { return ((uint64_t)(uint32_t)cx << 32) | (uint32_t)cz; }
inline int ckx(uint64_t k) { return (int)(uint32_t)(k >> 32); }
inline int ckz(uint64_t k) { return (int)(uint32_t)k; }

struct VillageInfo { bool ok = false; int cx = 0, cz = 0, y0 = 0, biome = 0; };
inline VillageInfo villageIn(uint32_t seed, int rx, int rz) {  // at most one village per 64x64 region, on flat land
  VillageInfo v;
  if (hash3(seed + 31, rx, 33, rz) >= .28f) return v;
  int cx = rx * 64 + 16 + (int)(hash3(seed + 32, rx, 34, rz) * 32), cz = rz * 64 + 16 + (int)(hash3(seed + 33, rx, 35, rz) * 32);
  int h = terrainH(seed, cx, cz);
  if (h <= SEA + 1 || h > 32) return v;
  int bi = biomeAt(seed, cx, cz, h);
  if (bi != PLAINS && bi != DESERT && bi != SNOWY && bi != FOREST) return v;
  static const int O[4][2] = {{14, 0}, {-14, 0}, {0, 14}, {0, -14}};
  for (auto& o : O) if (std::abs(terrainH(seed, cx + o[0], cz + o[1]) - h) > 4) return v;
  v.ok = true; v.cx = cx; v.cz = cz; v.y0 = h; v.biome = bi;
  return v;
}
constexpr int VH_X[6] = {-13, 13, -4, 6, -13, 13}, VH_Z[6] = {-4, -4, 13, 13, 10, 10};
template <class F> void villageBlocks(uint32_t seed, const VillageInfo& v, F&& put) {
  const int y0 = v.y0;
  const bool desert = v.biome == DESERT;
  const uint8_t wall = desert ? SANDSTONE : PLANKS, floorB = desert ? SANDSTONE : COBBLE, ground = desert ? SAND : v.biome == SNOWY ? SNOW : GRASS;
  for (int dx = -17; dx <= 17; dx++) for (int dz = -17; dz <= 17; dz++) {  // flatten and clear the site
    int x = v.cx + dx, z = v.cz + dz, th = terrainH(seed, x, z);
    for (int y = th + 1; y < y0; y++) put(x, y, z, desert ? SAND : DIRT);
    for (int y = y0 + 1; y <= y0 + 7; y++) put(x, y, z, AIR);
    put(x, y0, z, ground);
  }
  for (int k = 0; k < 6; k++) {  // gravel paths from the well to every door
    bool xd = std::abs(VH_X[k]) > std::abs(VH_Z[k]);
    int sx = xd ? (VH_X[k] > 0 ? -1 : 1) : 0, sz = xd ? 0 : (VH_Z[k] > 0 ? -1 : 1);
    int tx = v.cx + VH_X[k] + sx * 3, tz = v.cz + VH_Z[k] + sz * 3;
    for (int x = std::min(v.cx, tx); x <= std::max(v.cx, tx); x++) put(x, y0, v.cz, GRAVEL);
    for (int z = std::min(v.cz, tz); z <= std::max(v.cz, tz); z++) put(tx, y0, z, GRAVEL);
  }
  for (int dx = -1; dx <= 1; dx++) for (int dz = -1; dz <= 1; dz++) {  // the well
    put(v.cx + dx, y0, v.cz + dz, COBBLE);
    if (dx || dz) put(v.cx + dx, y0 + 1, v.cz + dz, COBBLE);
    if (dx && dz) put(v.cx + dx, y0 + 2, v.cz + dz, TORCH);
  }
  put(v.cx, y0, v.cz, WATER);
  for (int k = 0; k < 6; k++) {  // houses: 5x5, walls 3 high, plank roof, door gap facing the well, glass window opposite
    int hx = v.cx + VH_X[k], hz = v.cz + VH_Z[k];
    bool xd = std::abs(VH_X[k]) > std::abs(VH_Z[k]);
    int sx = xd ? (VH_X[k] > 0 ? -1 : 1) : 0, sz = xd ? 0 : (VH_Z[k] > 0 ? -1 : 1);
    for (int a = -2; a <= 2; a++) for (int b = -2; b <= 2; b++) {
      bool edge = std::abs(a) == 2 || std::abs(b) == 2;
      put(hx + a, y0, hz + b, floorB);
      for (int y = 1; y <= 3; y++) put(hx + a, y0 + y, hz + b, edge ? ((std::abs(a) == 2 && std::abs(b) == 2) ? (uint8_t)LOG : wall) : (uint8_t)AIR);
      put(hx + a, y0 + 4, hz + b, wall);
    }
    put(hx + sx * 2, y0 + 1, hz + sz * 2, AIR);
    put(hx + sx * 2, y0 + 2, hz + sz * 2, AIR);
    put(hx - sx * 2, y0 + 2, hz - sz * 2, GLASS);
    put(hx + 1, y0 + 1, hz + 1, TORCH);
    if (k == 0 || k == 3) put(hx - 1, y0 + 1, hz - 1, CHEST);  // village storage
  }
}
// Strongholds: three per world on a ring. A brick room with a 12-frame End portal, reached by a long stair tunnel from the surface.
inline void strongholdPos(uint32_t seed, int i, int& sx, int& sz) {
  float a = (i + hash3(seed + 41, i, 1, 0)) * 2.0943951f, r = 260 + hash3(seed + 42, i, 2, 0) * 140;
  sx = (int)(std::cos(a) * r); sz = (int)(std::sin(a) * r);
}
constexpr int SH_Y = 10;
template <class F> void strongholdBlocks(uint32_t seed, int sx, int sz, F&& put) {
  for (int a = -6; a <= 6; a++) for (int b = -6; b <= 6; b++) for (int y = SH_Y - 1; y <= SH_Y + 7; y++) {
    bool wall = std::abs(a) == 6 || std::abs(b) == 6 || y == SH_Y - 1 || y == SH_Y + 7;
    put(sx + a, y, sz + b, wall ? (hash3(seed, sx + a, y, sz + b) < .3f ? COBBLE : BRICKS) : AIR);
  }
  for (int a = -2; a <= 2; a++) for (int b = -2; b <= 2; b++)
    if (std::max(std::abs(a), std::abs(b)) == 2 && !(std::abs(a) == 2 && std::abs(b) == 2)) put(sx + a, SH_Y, sz + b, END_FRAME);
  put(sx + 4, SH_Y + 1, sz + 4, TORCH); put(sx - 4, SH_Y + 1, sz - 4, TORCH); put(sx + 4, SH_Y + 1, sz - 4, TORCH); put(sx - 4, SH_Y + 1, sz + 4, TORCH);
  for (int k = 0; k <= 70; k++) {  // stair tunnel climbing east, one block per step (walkable with auto step-up)
    int x = sx + 6 + k, y = SH_Y + k;
    for (int dz = -1; dz <= 1; dz++) { put(x, y - 1, sz + dz, COBBLE); for (int dy = 0; dy < 4; dy++) put(x, y + dy, sz + dz, AIR); }
    if (y >= terrainH(seed, x, sz) + 1 || y > WH - 6) break;
  }
}
struct World {
  uint32_t seed = 1;
  int dim = OVERWORLD;
  std::unordered_map<uint64_t, Chunk> ch;
  std::unordered_map<uint64_t, std::unordered_map<int, uint8_t>> edits[3];  // per dimension, per chunk: index -> block
  Chunk* lastC = nullptr;
  uint64_t lastK = ~0ull;
  // Fluids: block id WATER/LAVA; flowing cells keep a level 1..7 here, source blocks (level 8) have no entry.
  std::unordered_map<uint64_t, uint8_t> fl[3];
  std::unordered_set<uint64_t> pend;  // fluid cells that need re-evaluating (only these are simulated)
  static uint64_t pk(int x, int y, int z) { return ((uint64_t)(x & 0x1FFFFF) << 28) | ((uint64_t)(z & 0x1FFFFF) << 7) | (uint64_t)(y & 127); }
  static void unpk(uint64_t k, int& x, int& y, int& z) {
    x = (int)((k >> 28) & 0x1FFFFF); if (x & 0x100000) x -= 0x200000;
    z = (int)((k >> 7) & 0x1FFFFF); if (z & 0x100000) z -= 0x200000;
    y = (int)(k & 127);
  }
  int level(int x, int y, int z) {
    uint8_t b = at(x, y, z);
    if (b != WATER && b != LAVA) return 0;
    auto it = fl[dim].find(pk(x, y, z));
    return it == fl[dim].end() ? 8 : it->second;
  }
  void poke(int x, int y, int z) {
    static const int D[7][3] = {{0, 0, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (auto& d : D) {
      int a = x + d[0], b = y + d[1], c = z + d[2];
      if (b < 0 || b >= WH) continue;
      uint8_t t = at(a, b, c);
      if (t == WATER || t == LAVA) pend.insert(pk(a, b, c));
    }
  }
  void removeFluid(int x, int y, int z) { set(x, y, z, AIR); fl[dim].erase(pk(x, y, z)); }
  // Place fluid; water touching lava makes obsidian (source) / cobblestone (flowing); lava touching water makes stone/cobble.
  void putFluid(int x, int y, int z, uint8_t t, int lvl) {
    static const int D[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    if (t == WATER) {
      for (auto& d : D) if (at(x + d[0], y + d[1], z + d[2]) == LAVA) {
        int ll = level(x + d[0], y + d[1], z + d[2]);
        set(x + d[0], y + d[1], z + d[2], ll == 8 ? OBSIDIAN : COBBLE);
        fl[dim].erase(pk(x + d[0], y + d[1], z + d[2]));
      }
    } else {
      for (auto& d : D) if (at(x + d[0], y + d[1], z + d[2]) == WATER) { set(x, y, z, COBBLE); fl[dim].erase(pk(x, y, z)); return; }
    }
    set(x, y, z, t);
    if (lvl >= 8) fl[dim].erase(pk(x, y, z)); else fl[dim][pk(x, y, z)] = (uint8_t)lvl;
  }
  bool flowInto(int x, int y, int z, uint8_t t, int lvl) {
    uint8_t o = at(x, y, z);
    if (o == AIR) { putFluid(x, y, z, t, lvl); return true; }
    if (o == t) { int ol = level(x, y, z); if (ol < 8 && ol < lvl) { putFluid(x, y, z, t, lvl); return true; } return false; }
    if (t == WATER && o == LAVA) { int ll = level(x, y, z); set(x, y, z, ll == 8 ? OBSIDIAN : COBBLE); fl[dim].erase(pk(x, y, z)); return true; }
    if (t == LAVA && o == WATER) { set(x, y, z, STONE); fl[dim].erase(pk(x, y, z)); return true; }
    return false;
  }
  void fluidTick(bool water, bool lava, int budget = 300) {
    if (pend.empty()) return;
    static const int H4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    std::vector<uint64_t> cur(pend.begin(), pend.end());
    int done = 0;
    for (uint64_t k : cur) {
      int x, y, z;
      unpk(k, x, y, z);
      uint8_t t = at(x, y, z);
      if (t != WATER && t != LAVA) { pend.erase(k); continue; }
      if ((t == WATER && !water) || (t == LAVA && !lava)) continue;
      if (done++ >= budget) break;
      pend.erase(k);
      int L = level(x, y, z);
      if (L < 8) {  // flowing water needs something feeding it, otherwise it dries up
        bool fed = at(x, y + 1, z) == t;
        for (auto& d : H4) if (!fed && at(x + d[0], y, z + d[1]) == t && level(x + d[0], y, z + d[1]) > L) fed = true;
        if (!fed) { removeFluid(x, y, z); continue; }
      }
      uint8_t below = y > 0 ? at(x, y - 1, z) : (uint8_t)BEDROCK;
      uint8_t other = t == WATER ? LAVA : WATER;
      if (below == AIR || below == other) { flowInto(x, y - 1, z, t, t == LAVA ? 3 : 7); continue; }  // gravity first
      if (below == t && level(x, y - 1, z) < 8) continue;  // the cell below is itself flowing down
      int sideL = L == 8 ? (t == LAVA ? 3 : 7) : L - 1;     // lava spreads 3 blocks, water 7
      if (sideL >= 1) for (auto& d : H4) flowInto(x + d[0], y, z + d[1], t, sideL);
    }
  }

  Chunk& chunk(int cx, int cz) {
    uint64_t k = ckey(cx, cz);
    if (k == lastK && lastC) return *lastC;
    auto it = ch.find(k);
    if (it == ch.end()) {
      it = ch.emplace(k, Chunk()).first;
      gen(it->second, cx, cz);
    }
    lastK = k; lastC = &it->second;
    return it->second;
  }
  bool loaded(int cx, int cz) const { return ch.count(ckey(cx, cz)) > 0; }
  uint8_t at(int x, int y, int z) {
    if (y < 0) return dim == END ? AIR : BEDROCK;
    if (y >= WH) return AIR;
    return chunk(x >> 4, z >> 4).d[idx(x & 15, y, z & 15)];
  }
  void bump(int cx, int cz) { auto it = ch.find(ckey(cx, cz)); if (it != ch.end()) it->second.ver++; }
  void set(int x, int y, int z, uint8_t b, bool rec = true) {
    if (y < 0 || y >= WH) return;
    int cx = x >> 4, cz = z >> 4;
    Chunk& c = chunk(cx, cz);
    int i = idx(x & 15, y, z & 15);
    if (c.d[i] == b) return;
    c.d[i] = b; c.ver++;
    if (rec) { edits[dim][ckey(cx, cz)][i] = b; poke(x, y, z); }
    if ((x & 15) == 0) bump(cx - 1, cz);
    if ((x & 15) == 15) bump(cx + 1, cz);
    if ((z & 15) == 0) bump(cx, cz - 1);
    if ((z & 15) == 15) bump(cx, cz + 1);
  }
  void unloadFar(int pcx, int pcz, int r) {
    for (auto it = ch.begin(); it != ch.end();) {
      if (std::abs(ckx(it->first) - pcx) > r || std::abs(ckz(it->first) - pcz) > r) it = ch.erase(it);
      else ++it;
    }
    lastC = nullptr; lastK = ~0ull;
  }
  void reset(int newDim) { ch.clear(); lastC = nullptr; lastK = ~0ull; dim = newDim; pend.clear(); }

  void gen(Chunk& c, int cx, int cz) {
    if (dim == OVERWORLD) genOver(c, cx, cz);
    else if (dim == NETHER) genNether(c, cx, cz);
    else genEnd(c, cx, cz);
    if (dim == OVERWORLD) structures(c, cx, cz);
    auto e = edits[dim].find(ckey(cx, cz));
    if (e != edits[dim].end()) for (auto& kv : e->second) c.d[kv.first] = kv.second;
  }
  void genOver(Chunk& c, int cx, int cz) {
    uint32_t s = seed;
    for (int x = 0; x < CS; x++)
      for (int z = 0; z < CS; z++) {
        int wx = cx * CS + x, wz = cz * CS + z, h = terrainH(s, wx, wz), bi = biomeAt(s, wx, wz, h);
        bool sandy = bi == DESERT || bi == BEACH || bi == OCEAN;
        uint8_t topB = sandy ? SAND : bi == SNOWY ? SNOW : bi == MOUNTAINS ? (h > 46 ? SNOW : STONE) : GRASS;
        if (bi == OCEAN) topB = hash3(s, wx, 3, wz) < .4f ? GRAVEL : SAND;
        uint8_t subB = sandy ? SAND : bi == MOUNTAINS ? STONE : DIRT;
        for (int y = 0; y <= std::max(h, SEA); y++) {
          uint8_t b = AIR;
          if (y == 0) b = BEDROCK;
          else if (y <= h) {
            if (y < h - 3) b = STONE; else b = (y == h) ? topB : subB;
            if (bi == DESERT && y < h - 3 && y >= h - 6) b = SANDSTONE;
            if (b == STONE) {
              float r = hash3(s, wx, y, wz);
              b = r < .003f && y < 12 ? DIAMOND_ORE : r < .008f && y < 18 ? GOLD_ORE : r < .018f && y < 34 ? IRON_ORE : r < .03f ? COAL_ORE : STONE;
            }
            if (y > 2 && y < h - 2 && std::fabs(vnoise(s + 9, wx + y * 11.f, wz + y * 7.f, 9) - .5f) < .07f &&
                std::fabs(vnoise(s + 10, wz - y * 5.f, wx + y * 13.f, 9) - .5f) < .07f)
              b = y < 6 ? LAVA : AIR;  // winding caves
          } else b = (y == SEA && bi == SNOWY) ? ICE : WATER;
          c.d[idx(x, y, z)] = b;
        }
      }
    for (int lx = 3; lx < CS - 3; lx++)
      for (int lz = 3; lz < CS - 3; lz++) {
        int wx = cx * CS + lx, wz = cz * CS + lz, h = terrainH(s, wx, wz);
        if (h <= SEA + 1) continue;
        int bi = biomeAt(s, wx, wz, h);
        float dens = bi == FOREST ? .05f : bi == PLAINS ? .006f : bi == SNOWY ? .012f : 0;
        if (hash3(s, wx, 5, wz) >= dens) continue;
        uint8_t tb = c.d[idx(lx, h, lz)];
        if (tb != GRASS && tb != SNOW) continue;
        int tall = 4 + (int)(hash3(s, wx, 6, wz) * 3), r = hash3(s, wx, 7, wz) > .7f ? 3 : 2;
        for (int y = 1; y <= tall && h + y < WH; y++) c.d[idx(lx, h + y, lz)] = LOG;
        for (int dy = tall - 2; dy <= tall + 1; dy++) {
          int rr = dy > tall - 1 ? 1 : r;
          for (int a = -rr; a <= rr; a++)
            for (int b = -rr; b <= rr; b++) {
              int yy = h + dy;
              if (a * a + b * b > rr * rr + 1 || yy >= WH) continue;
              if (c.d[idx(lx + a, yy, lz + b)] == AIR) c.d[idx(lx + a, yy, lz + b)] = LEAVES;
            }
        }
      }
  }
  void structures(Chunk& c, int cx, int cz) {
    auto putc = [&](int x, int y, int z, uint8_t b) {
      int lx = x - cx * CS, lz = z - cz * CS;
      if (lx < 0 || lx >= CS || lz < 0 || lz >= CS || y < 0 || y >= WH) return;
      c.d[idx(lx, y, lz)] = b;
    };
    int x0 = cx * CS, z0 = cz * CS;
    for (int rx = (int)std::floor((x0 - 18) / 64.f); rx <= (int)std::floor((x0 + CS + 18) / 64.f); rx++)
      for (int rz = (int)std::floor((z0 - 18) / 64.f); rz <= (int)std::floor((z0 + CS + 18) / 64.f); rz++) {
        VillageInfo v = villageIn(seed, rx, rz);
        if (v.ok) villageBlocks(seed, v, putc);
      }
    for (int i = 0; i < 3; i++) {
      int sx, sz;
      strongholdPos(seed, i, sx, sz);
      if (x0 + CS > sx - 7 && x0 < sx + 80 && z0 + CS > sz - 7 && z0 < sz + 7) strongholdBlocks(seed, sx, sz, putc);
    }
  }
  void genNether(Chunk& c, int cx, int cz) {
    for (int x = 0; x < CS; x++)
      for (int z = 0; z < CS; z++) {
        int wx = cx * CS + x, wz = cz * CS + z;
        for (int y = 0; y < WH; y++) {
          uint8_t b = AIR;
          if (y == 0 || y == WH - 1) b = BEDROCK;
          else {
            float n = vnoise(seed, wx + y * 4.1f, wz - y * 3.3f, 14) * .6f + vnoise(seed + 1, wz + y * 6.7f, wx + y * 2.2f, 6) * .4f;
            if (n < .48f || y < 5 || y > WH - 7) {
              b = NETHERRACK;
              if (y < 10 && vnoise(seed + 2, (float)wx, (float)wz, 9) > .6f) b = SOULSAND;
              else if (hash3(seed, wx, y, wz) < .01f) b = GLOWSTONE;
            } else if (y <= 9) b = LAVA;
          }
          c.d[idx(x, y, z)] = b;
        }
      }
  }
  void genEnd(Chunk& c, int cx, int cz) {
    for (int x = 0; x < CS; x++)
      for (int z = 0; z < CS; z++) {
        int wx = cx * CS + x, wz = cz * CS + z;
        float dd = std::hypot((float)wx, (float)wz), t = std::max(0.f, 1 - dd / 42) * 14, n = vnoise(seed, wx + 1000.f, (float)wz, 18);
        if (n > .72f && dd > 55) t = std::max(t, (n - .72f) * 25);
        if (t > 0) for (int y = std::max(1, (int)(32 - t)); y <= 32; y++) c.d[idx(x, y, z)] = ENDSTONE;
        for (int i = 0; i < 8; i++) {
          float a = i * 0.785398f;
          if (wx == (int)std::lround(std::cos(a) * 20) && wz == (int)std::lround(std::sin(a) * 20)) for (int y = 33; y <= 32 + 6 + (i % 3) * 3; y++) c.d[idx(x, y, z)] = OBSIDIAN;
        }
      }
  }
};

inline void findSpawn(World& w, float& px, float& py, float& pz) {
  if (w.dim == OVERWORLD) {
    for (int r = 0; r < 900; r += 3) {
      int x = r, z = r / 2, h = terrainH(w.seed, x, z);
      if (h >= SEA + 2 && biomeAt(w.seed, x, z, h) != OCEAN) { px = x + .5f; py = h + 1.01f; pz = z + .5f; return; }
    }
    px = .5f; py = 40; pz = .5f; return;
  }
  if (w.dim == END) {
    px = pz = .5f; py = 34;
    for (int y = WH - 1; y > 0; y--) if (binfo(w.at(0, y, 0)).solid) { py = y + 1.01f; break; }
    return;
  }
  for (int r = 0; r < 48; r += 2)
    for (int y = 10; y < 57; y++) {
      uint8_t b = w.at(r, y - 1, r), a = w.at(r, y, r), a2 = w.at(r, y + 1, r);
      if (binfo(b).solid && !binfo(a).solid && !binfo(a2).solid && a != LAVA && a2 != LAVA) { px = r + .5f; py = (float)y + .01f; pz = r + .5f; return; }
    }
  for (int a = -1; a <= 1; a++) for (int b = -1; b <= 1; b++) { w.set(a, 29, b, NETHERRACK); w.set(a, 30, b, AIR); w.set(a, 31, b, AIR); }
  px = pz = .5f; py = 30.01f;
}

// ---------- inventory / crafting / furnace ----------
struct Stack {
  int id = 0, n = 0, dur = 0;
  bool empty() const { return id <= 0 || n <= 0; }
};
inline Stack mk(int id, int n) {
  Stack s; s.id = id; s.n = n; s.dur = maxDur(id); return s;
}
struct Inv {
  Stack s[36];  // 0..8 hotbar
  int add(int id, int n, int dur = -1) {
    if (maxStack(id) > 1)
      for (int i = 0; i < 36 && n > 0; i++)
        if (s[i].id == id && s[i].n < maxStack(id)) { int a = std::min(n, maxStack(id) - s[i].n); s[i].n += a; n -= a; }
    for (int i = 0; i < 36 && n > 0; i++)
      if (s[i].empty()) { int a = std::min(n, maxStack(id)); s[i] = mk(id, a); if (dur >= 0) s[i].dur = dur; n -= a; }
    return n;
  }
  int count(int id) const { int c = 0; for (auto& x : s) if (!x.empty() && x.id == id) c += x.n; return c; }
  void take(int id, int n) {
    for (auto& x : s) if (!x.empty() && x.id == id && n > 0) { int a = std::min(n, x.n); x.n -= a; n -= a; if (x.n <= 0) x = Stack(); }
  }
};

struct Rec {
  int w = 0, h = 0, cell[9] = {0}, out = 0, n = 1;
  bool shapeless = false;
  bool table() const { return w > 2 || h > 2; }
};
inline int keyItem(char c) {
  switch (c) {
    case 'P': return PLANKS; case 'S': return I_STICK; case 'C': return COBBLE; case 'I': return I_IRON;
    case 'G': return I_GOLD; case 'D': return I_DIAMOND; case 'L': return LOG; case 'N': return SAND;
    case 'K': return I_COAL; case 'T': return I_LEATHER; case 'W': return I_STRING; case 'F': return I_FLINT; case 'E': return I_FEATHER;
    default: return 0;
  }
}
inline Rec shaped(std::initializer_list<const char*> rows, int out, int n = 1) {
  Rec r; r.out = out; r.n = n;
  for (auto row : rows) { r.w = std::max<int>(r.w, (int)std::strlen(row)); r.h++; }
  int y = 0;
  for (auto row : rows) { for (int x = 0; row[x]; x++) r.cell[y * r.w + x] = keyItem(row[x]); y++; }
  return r;
}
inline const std::vector<Rec>& recipes() {
  static std::vector<Rec> R;
  if (R.empty()) {
    { Rec r; r.shapeless = true; r.w = 1; r.h = 1; r.cell[0] = LOG; r.out = PLANKS; r.n = 4; R.push_back(r); }
    R.push_back(shaped({"P", "P"}, I_STICK, 4));
    R.push_back(shaped({"PP", "PP"}, TABLE));
    R.push_back(shaped({"NN", "NN"}, SANDSTONE));
    R.push_back(shaped({"CCC", "C C", "CCC"}, FURNACE));
    const char mat[5] = {'P', 'C', 'I', 'G', 'D'};
    for (int t = 0; t < 5; t++) {
      char m = mat[t];
      char a1[4] = {m, m, m, 0}, a2[4] = {' ', 'S', ' ', 0}, a3[4] = {' ', 'S', ' ', 0};
      R.push_back(shaped({a1, a2, a3}, I_TOOL0 + t));
      char b1[3] = {m, m, 0}, b2[3] = {m, 'S', 0}, b3[3] = {' ', 'S', 0};
      R.push_back(shaped({b1, b2, b3}, I_TOOL0 + 5 + t));
      char c1[2] = {m, 0}, c2[2] = {'S', 0}, c3[2] = {'S', 0};
      R.push_back(shaped({c1, c2, c3}, I_TOOL0 + 10 + t));
      char d1[2] = {m, 0}, d2[2] = {m, 0}, d3[2] = {'S', 0};
      R.push_back(shaped({d1, d2, d3}, I_TOOL0 + 15 + t));
      char e1[3] = {m, m, 0}, e2[3] = {' ', 'S', 0}, e3[3] = {' ', 'S', 0};
      R.push_back(shaped({e1, e2, e3}, I_TOOL0 + 20 + t));
    }
    const char am[4] = {'T', 'I', 'G', 'D'};
    for (int m = 0; m < 4; m++) {
      char c = am[m];
      char h1[4] = {c, c, c, 0}, h2[4] = {c, ' ', c, 0};
      R.push_back(shaped({h1, h2}, I_ARMOR0 + m * 4 + 0));
      char k1[4] = {c, ' ', c, 0}, k2[4] = {c, c, c, 0}, k3[4] = {c, c, c, 0};
      R.push_back(shaped({k1, k2, k3}, I_ARMOR0 + m * 4 + 1));
      char l1[4] = {c, c, c, 0}, l2[4] = {c, ' ', c, 0}, l3[4] = {c, ' ', c, 0};
      R.push_back(shaped({l1, l2, l3}, I_ARMOR0 + m * 4 + 2));
      char b1[4] = {c, ' ', c, 0}, b2[4] = {c, ' ', c, 0};
      R.push_back(shaped({b1, b2}, I_ARMOR0 + m * 4 + 3));
    }
    R.push_back(shaped({" SW", "S W", " SW"}, I_BOW));
    R.push_back(shaped({"F", "S", "E"}, I_ARROW, 4));
    R.push_back(shaped({"I I", " I "}, I_BUCKET));
    { Rec r; r.shapeless = true; r.w = 2; r.h = 1; r.cell[0] = I_IRON; r.cell[1] = I_FLINT; r.out = I_FLINT_STEEL; R.push_back(r); }
    auto sl = [&](int a, int b, int out, int n = 1) { Rec r; r.shapeless = true; r.w = 2; r.h = 1; r.cell[0] = a; r.cell[1] = b; r.out = out; r.n = n; R.push_back(r); };
    R.push_back(shaped({"K", "S"}, TORCH, 4));
    R.push_back(shaped({"PPP", "P P", "PPP"}, CHEST));
    sl(I_IRON, I_COAL, WIRE, 4);
    R.push_back(shaped({"S", "C"}, LEVER));
    R.push_back(shaped({"C"}, BUTTON));
    R.push_back(shaped({"PP"}, PLATE));
    sl(GLASS, WIRE, LAMP);
    R.push_back(shaped({"PP", "PP", "PP"}, DOOR));
    sl(I_PEARL, I_EMBER, I_EYE);
    R.push_back(shaped({"PIP", "PPP", " P "}, I_SHIELD));
  }
  return R;
}
// Match a crafting grid (gw x gh) against recipes. Returns result item id (0 = none).
inline int matchGrid(const Stack* g, int gw, int gh, int* outN) {
  int x0 = gw, x1 = -1, y0 = gh, y1 = -1;
  std::vector<int> items;
  for (int y = 0; y < gh; y++)
    for (int x = 0; x < gw; x++)
      if (!g[y * gw + x].empty()) { x0 = std::min(x0, x); x1 = std::max(x1, x); y0 = std::min(y0, y); y1 = std::max(y1, y); items.push_back(g[y * gw + x].id); }
  if (x1 < 0) return 0;
  int w = x1 - x0 + 1, h = y1 - y0 + 1;
  std::sort(items.begin(), items.end());
  for (const Rec& r : recipes()) {
    if (r.shapeless) {
      std::vector<int> b;
      for (int i = 0; i < r.w * r.h; i++) b.push_back(r.cell[i]);
      std::sort(b.begin(), b.end());
      if (b == items) { *outN = r.n; return r.out; }
      continue;
    }
    if (r.w != w || r.h != h) continue;
    for (int mir = 0; mir < 2; mir++) {
      bool ok = true;
      for (int y = 0; y < h && ok; y++)
        for (int x = 0; x < w; x++) {
          int rc = r.cell[y * w + (mir ? w - 1 - x : x)];
          const Stack& s = g[(y0 + y) * gw + x0 + x];
          if ((s.empty() ? 0 : s.id) != rc) { ok = false; break; }
        }
      if (ok) { *outN = r.n; return r.out; }
    }
  }
  return 0;
}
inline void recNeeds(const Rec& r, std::vector<std::pair<int, int>>& need) {
  need.clear();
  for (int i = 0; i < r.w * r.h; i++) {
    if (!r.cell[i]) continue;
    bool f = false;
    for (auto& p : need) if (p.first == r.cell[i]) { p.second++; f = true; }
    if (!f) need.push_back({r.cell[i], 1});
  }
}
inline bool canAuto(const Inv& inv, const Rec& r) {
  std::vector<std::pair<int, int>> need; recNeeds(r, need);
  for (auto& p : need) if (inv.count(p.first) < p.second) return false;
  return true;
}
inline bool autoCraft(Inv& inv, const Rec& r) {
  if (!canAuto(inv, r)) return false;
  Inv bak = inv;
  std::vector<std::pair<int, int>> need; recNeeds(r, need);
  for (auto& p : need) inv.take(p.first, p.second);
  if (inv.add(r.out, r.n) > 0) { inv = bak; return false; }
  return true;
}

inline int smeltOf(int id) {
  switch (id) { case IRON_ORE: return I_IRON; case GOLD_ORE: return I_GOLD; case SAND: return GLASS; case COBBLE: return STONE; case LOG: return I_COAL;
    case I_RAW_BEEF: return I_STEAK; case I_RAW_PORK: return I_PORK; case I_RAW_CHICKEN: return I_CHICKEN; default: return 0; }
}
inline float fuelOf(int id) {
  switch (id) { case I_COAL: return 80; case LOG: return 15; case PLANKS: return 7.5f; case I_STICK: return 5; default: return 0; }
}
struct Furn {
  int x = 0, y = 0, z = 0, dim = 0;
  Stack in, fuel, out;
  float burn = 0, burnMax = 1, prog = 0;
};
inline void furnTick(Furn& f, float dt) {
  int res = f.in.empty() ? 0 : smeltOf(f.in.id);
  bool can = res && (f.out.empty() || (f.out.id == res && f.out.n < 64));
  if (can && f.burn <= 0 && !f.fuel.empty() && fuelOf(f.fuel.id) > 0) {
    f.burnMax = f.burn = fuelOf(f.fuel.id);
    if (--f.fuel.n <= 0) f.fuel = Stack();
  }
  if (f.burn > 0) {
    f.burn = std::max(0.f, f.burn - dt);
    if (can) {
      f.prog += dt;
      if (f.prog >= 10) {
        f.prog = 0;
        if (--f.in.n <= 0) f.in = Stack();
        if (f.out.empty()) f.out = mk(res, 1); else f.out.n++;
      }
    } else f.prog = std::max(0.f, f.prog - dt * 2);
  } else f.prog = std::max(0.f, f.prog - dt * 2);
}

// ---------- game state ----------
struct Input { float fwd = 0, right = 0; bool jump = false, sprint = false, sneak = false; };
struct Hit { bool ok = false; int x = 0, y = 0, z = 0, nx = 0, ny = 0, nz = 0, b = 0; };
struct Vitals { float hp = 20, food = 20, sat = 5, exh = 0, air = 15, regen = 0, drownT = 0, hurtT = 0; };
struct Pile { float x = 0, y = 0, z = 0; int dim = 0; std::vector<Stack> items; };
enum MobType { M_COW, M_PIG, M_SHEEP, M_CHICKEN, M_ZOMBIE, M_SKELETON, M_SPIDER, M_CREEPER, M_CINDER, M_VILLAGER, M_WARPER, M_DRAGON, M_CRYSTAL, M_COUNT };
struct MobInfo { const char* name; float hp, speed, w, h; bool hostile; float dmg; };
inline const MobInfo& minfo(int t) {
  static const MobInfo T[M_COUNT] = {
      {"Cow", 10, 1.6f, .9f, 1.3f, false, 0},       {"Pig", 10, 1.7f, .9f, .9f, false, 0},          {"Sheep", 8, 1.7f, .9f, 1.2f, false, 0},
      {"Chicken", 4, 1.5f, .4f, .7f, false, 0},     {"Shambler", 20, 2.5f, .6f, 1.8f, true, 3},     {"Bone Archer", 20, 2.3f, .6f, 1.8f, true, 2},
      {"Crawler", 16, 3.4f, 1.1f, .8f, true, 2},    {"Fuse Beast", 20, 2.5f, .6f, 1.6f, true, 0},   {"Cinder Walker", 22, 2.8f, .6f, 1.8f, true, 4},
      {"Villager", 20, 1.4f, .6f, 1.8f, false, 0},    {"Warper", 40, 3.2f, .6f, 2.8f, true, 5},       {"Sky Wyrm", 200, 0, 3.f, 2.f, true, 0},
      {"Sky Crystal", 4, 0, .8f, .8f, false, 0}};
  return T[t >= 0 && t < M_COUNT ? t : 0];
}
struct Mob { int type = 0; float x = 0, y = 0, z = 0, vx = 0, vy = 0, vz = 0, yaw = 0, hp = 10, atk = 0, ai = 0, tx = 0, tz = 0, hurt = 0, fuse = -1, anim = 0, flee = 0, hx = 0, hz = 0, t1 = 0, t2 = 0; bool ground = false; int prof = 0, uses[8] = {0}; };
struct Arrow { float x = 0, y = 0, z = 0, vx = 0, vy = 0, vz = 0, life = 6, dmg = 3; bool mine = false; };
struct ChestData { int x = 0, y = 0, z = 0, dim = 0; Stack s[27]; };
struct BtnTimer { int x, y, z; float t; };
struct Trade { int give, giveN, get, getN, max; };
enum Ev { EV_BREAK, EV_PLACE, EV_PICKUP, EV_HURT, EV_ATTACK, EV_MOBHURT, EV_EXPLODE, EV_PORTAL, EV_DOOR, EV_CLICK, EV_SHOOT, EV_THUNDER, EV_EAT, EV_DRAGON, EV_CRAFT, EV_COUNT };
struct Meta { std::string id, name; int mode = 0, dim = 0; long long seed = 0, played = 0; bool dead = false; };

inline const std::vector<Trade>& trades(int prof) {
  static std::vector<std::vector<Trade>> T;
  if (T.empty()) {
    T.push_back({{I_APPLE, 8, I_EMERALD, 1, 8}, {I_EMERALD, 1, I_CHICKEN, 4, 8}, {I_EMERALD, 1, I_APPLE, 6, 8}});  // Farmer
    T.push_back({{I_RAW_BEEF, 8, I_EMERALD, 1, 8}, {I_RAW_PORK, 8, I_EMERALD, 1, 8}, {I_EMERALD, 1, I_STEAK, 4, 8}, {I_EMERALD, 1, I_PORK, 4, 8}});  // Butcher
    T.push_back({{I_COAL, 12, I_EMERALD, 1, 8}, {I_IRON, 4, I_EMERALD, 1, 8}, {I_EMERALD, 2, I_TOOL0 + 1, 1, 3}, {I_EMERALD, 6, I_TOOL0 + 2, 1, 2}, {I_EMERALD, 5, I_TOOL0 + 7, 1, 2}});  // Toolsmith
    T.push_back({{I_IRON, 4, I_EMERALD, 1, 8}, {I_EMERALD, 3, I_ARMOR0 + 4, 1, 3}, {I_EMERALD, 7, I_ARMOR0 + 5, 1, 2}, {I_EMERALD, 5, I_ARMOR0 + 6, 1, 2}, {I_EMERALD, 3, I_ARMOR0 + 7, 1, 3}});  // Armorer
    T.push_back({{I_ROTTEN, 16, I_EMERALD, 1, 8}, {I_EMERALD, 5, I_PEARL, 1, 3}, {I_EMERALD, 2, I_FLINT_STEEL, 1, 2}, {I_BONE, 10, I_EMERALD, 1, 8}});  // Cleric
    T.push_back({{I_STRING, 10, I_EMERALD, 1, 8}, {I_FEATHER, 10, I_EMERALD, 1, 8}, {I_EMERALD, 1, I_ARROW, 8, 8}, {I_EMERALD, 3, I_BOW, 1, 3}});  // Fletcher
  }
  return T[(prof % 6 + 6) % 6];
}
inline const char* profName(int p) { static const char* n[] = {"Farmer", "Butcher", "Toolsmith", "Armorer", "Cleric", "Fletcher"}; return n[(p % 6 + 6) % 6]; }
struct Game {
  World w;
  int mode = SURVIVAL, diff = 2;
  std::string name = "World", id = "w0";
  Inv inv;
  int sel = 0;
  Vitals v;
  float x = 0, y = 40, z = 0, vx = 0, vy = 0, vz = 0, yaw = 0, pitch = 0;
  bool ground = false, wet = false, headWet = false, flying = false, dead = false, locked = false, sneak = false;
  float fallPeak = 40, time = 120, walkT = 0, eatT = 0;
  float dpos[3][3] = {{0}};
  bool dposSet[3] = {false, false, false};
  std::vector<Furn> furn;
  std::vector<Pile> piles;
  long long played = 0;
  int mx = 0, my = 0, mz = 0;
  float mprog = 0;
  bool mActive = false;
  Stack armor[4];  // helmet, chestplate, leggings, boots
  std::vector<Mob> mobs;
  std::vector<Arrow> arrows;
  float spawnT = 1, waterT = 0, lavaT = 0, atkT = 0, bowT = 0, portalT = 0;
  bool portalCool = false;
  uint32_t rs = 2463534242u;
  float rnd() { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return (rs & 0xFFFFFF) / 16777216.f; }
  std::vector<ChestData> chests;
  std::vector<BtnTimer> timers;
  std::unordered_set<uint64_t> doorsOpen;
  int plateX = 0, plateY = 0, plateZ = 0;
  bool plateOn = false, blocking = false;
  int weather = 0;  // 0 clear, 1 rain/snow, 2 thunderstorm
  float weatherT = 240, strikeT = 10, flash = 0, strikeX = 0, strikeZ = 0, strikeShow = 0;
  std::vector<int> events;  // sound/effect events for the frontend to play
  std::string note;         // short message for the player
  float noteT = 0;
  bool dragonDead = false, dragonSpawned = false, victory = false;
  struct EyeFlight { float x, y, z, dx, dz, t; };
  std::vector<EyeFlight> eyes;
  void emit(int e) { if (events.size() < 64) events.push_back(e); }
  void say(const std::string& m, float t = 3) { note = m; noteT = t; }

  void newWorld(const std::string& nm, uint32_t seed, int m, int d) {
    *this = Game();
    name = nm; w.seed = seed; mode = m; diff = (m == HARDCORE) ? 3 : d;
    findSpawn(w, x, y, z); fallPeak = y;
  }
  void toSpawn() { findSpawn(w, x, y, z); vx = vy = vz = 0; fallPeak = y; }
  void changeDim(int d) {
    dpos[w.dim][0] = x; dpos[w.dim][1] = y; dpos[w.dim][2] = z; dposSet[w.dim] = true;
    w.reset(d);
    mobs.clear(); arrows.clear();
    if (dposSet[d]) { x = dpos[d][0]; y = dpos[d][1]; z = dpos[d][2]; } else findSpawn(w, x, y, z);
    vx = vy = vz = 0; fallPeak = y;
    if (d == END && !dragonDead) spawnDragon();
  }
  float eyeH() const { return sneak ? 1.45f : 1.62f; }
  Stack& held() { return inv.s[sel]; }
  float daylight() const {
    if (w.dim != OVERWORLD) return .6f;
    float s = std::sin(time / DAYLEN * 6.2831853f);
    return std::max(0.f, std::min(1.f, s * 2 + .5f));
  }

  bool hitBox(float px, float py, float pz) {
    int x0 = (int)std::floor(px - .3f), x1 = (int)std::floor(px + .3f), y0 = (int)std::floor(py), y1 = (int)std::floor(py + 1.79f),
        z0 = (int)std::floor(pz - .3f), z1 = (int)std::floor(pz + .3f);
    for (int a = x0; a <= x1; a++) for (int b = y0; b <= y1; b++) for (int c = z0; c <= z1; c++) if (binfo(w.at(a, b, c)).solid) return true;
    return false;
  }
  bool moveAxis(int ax, float d) {  // returns true when blocked
    float* p[3] = {&x, &y, &z};
    float* vv[3] = {&vx, &vy, &vz};
    *p[ax] += d;
    if (!hitBox(x, y, z)) return false;
    if (ax != 1 && ground) {  // step-up one block
      float oy = y;
      y = std::floor(y) + 1;
      if (!hitBox(x, y, z)) return false;
      y = oy;
    }
    *p[ax] -= d; *vv[ax] = 0;
    if (ax == 1 && d < 0) ground = true;
    return true;
  }
  void stepAxis(int ax, float d) {
    int n = std::max(1, (int)std::ceil(std::fabs(d) / .4f));
    for (int i = 0; i < n; i++) if (moveAxis(ax, d / n)) break;
  }
  void addExh(float e) { v.exh += e; }
  void hurt(float n) {
    if (mode == CREATIVE || locked || dead || v.hurtT > 0) return;
    v.hp -= n; v.hurtT = .5f; emit(EV_HURT);
    if (v.hp <= 0) { v.hp = 0; die(); }
  }
  void die() {
    dead = true;
    if (mode == HARDCORE) { locked = true; return; }
    Pile p; p.x = x; p.y = y; p.z = z; p.dim = w.dim;
    for (auto& s : inv.s) if (!s.empty()) { p.items.push_back(s); s = Stack(); }
    for (auto& s : armor) if (!s.empty()) { p.items.push_back(s); s = Stack(); }
    if (!p.items.empty()) piles.push_back(p);
  }
  void respawn() {
    if (mode == HARDCORE) { dead = false; flying = true; locked = true; return; }  // spectate the dead world
    if (w.dim != OVERWORLD) { w.reset(OVERWORLD); mobs.clear(); arrows.clear(); }
    v = Vitals(); dead = false; flying = false; toSpawn();
  }

  void update(float dt, const Input& in) {
    time = std::fmod(time + dt, DAYLEN);
    for (auto& f : furn) furnTick(f, dt);
    worldTick(dt);
    if (dead) return;
    v.hurtT -= dt;
    sneak = in.sneak;
    bool creative = mode == CREATIVE || locked;
    if (locked) {  // spectator: free fly, no collision
      float fx = -std::sin(yaw), fz = std::cos(yaw), rx = -std::cos(yaw), rz = -std::sin(yaw);
      float sp = in.sprint ? 14.f : 8.f;
      x += (fx * in.fwd + rx * in.right) * sp * dt; z += (fz * in.fwd + rz * in.right) * sp * dt;
      y += ((in.jump ? 1.f : 0.f) - (in.sneak ? 1.f : 0.f)) * sp * dt;
      return;
    }
    for (int i = 0; i < 12 && hitBox(x, y, z); i++) y += 1;
    int bx = (int)std::floor(x), bz = (int)std::floor(z);
    uint8_t feet = w.at(bx, (int)std::floor(y + .4f), bz);
    wet = feet == WATER || feet == LAVA;
    headWet = w.at(bx, (int)std::floor(y + 1.6f), bz) == WATER;
    bool inLava = feet == LAVA || w.at(bx, (int)std::floor(y + .1f), bz) == LAVA;
    bool canSprint = in.sprint && in.fwd > 0 && (creative || v.food > 6);
    float sp = in.sneak ? 1.8f : (canSprint ? 5.8f : 4.2f);
    if (creative && flying) sp = in.sprint ? 12.f : 7.f;
    if (wet && !flying) sp *= .55f;
    float fx = -std::sin(yaw), fz = std::cos(yaw), rx = -std::cos(yaw), rz = -std::sin(yaw);
    float f = in.fwd, r = in.right, l = std::hypot(f, r);
    if (l > 1) { f /= l; r /= l; }
    float tx = (fx * f + rx * r) * sp, tz = (fz * f + rz * r) * sp, k = std::min(1.f, dt * (ground || flying ? 12.f : 3.5f));
    vx += (tx - vx) * k; vz += (tz - vz) * k;
    if (creative && flying) vy = (in.jump ? 7.f : 0.f) - (in.sneak ? 7.f : 0.f);
    else if (wet) { vy -= 6 * dt; if (in.jump) vy += 20 * dt; if (in.sneak) vy -= 10 * dt; vy = std::max(-3.f, std::min(3.5f, vy)); }
    else { vy = std::max(-32.f, vy - 28 * dt); if (in.jump && ground) { vy = 8.6f; addExh(.05f); } }
    stepAxis(0, vx * dt); stepAxis(2, vz * dt);
    ground = false;
    stepAxis(1, vy * dt);
    walkT += std::hypot(vx, vz) * dt;
    portalTick(dt);
    // falling damage
    if (!ground && !wet && !(creative && flying)) fallPeak = std::max(fallPeak, y);
    else { if (ground && fallPeak - y > 3.f && !creative) hurt(std::floor(fallPeak - y - 3.f)); fallPeak = y; }
    if (y < -30) { toSpawn(); if (!creative) hurt(4); }
    if (creative) { v.hp = 20; v.food = 20; v.air = 15; }
    else {
      if (inLava) hurt(4);
      if (headWet) { v.air -= dt; if (v.air <= 0) { v.air = 0; v.drownT += dt; if (v.drownT >= 1) { v.drownT = 0; hurt(2); } } }
      else { v.air = std::min(15.f, v.air + dt * 6); v.drownT = 0; }
      if (diff > 0) {
        if (canSprint) v.exh += std::hypot(vx, vz) * dt * .1f;
        if (v.exh >= 4) { v.exh -= 4; if (v.sat > 0) v.sat = std::max(0.f, v.sat - 1); else v.food = std::max(0.f, v.food - 1); }
        if (v.food >= 18 && v.hp < 20) { v.regen += dt; if (v.regen >= 4) { v.regen = 0; v.hp = std::min(20.f, v.hp + 1); v.exh += 6; } }
        else if (v.food <= 0) {
          v.regen += dt;
          if (v.regen >= 4) { v.regen = 0; if ((diff == 1 && v.hp > 10) || (diff == 2 && v.hp > 1) || diff == 3) hurt(1); }
        } else v.regen = 0;
      }
    }
    for (size_t i = 0; i < piles.size();) {  // pick up death piles
      Pile& p = piles[i];
      if (p.dim == w.dim && std::hypot(std::hypot(p.x - x, p.z - z), p.y - y) < 1.8f) {
        std::vector<Stack> rest;
        for (auto& s : p.items) { int left = inv.add(s.id, s.n, s.dur); if (left > 0) { Stack r = s; r.n = left; rest.push_back(r); } }
        if (rest.empty()) piles.erase(piles.begin() + i);
        else { p.items = rest; i++; }
      } else i++;
    }
  }

  Hit raycast(float maxd = 5.5f, bool fluids = false) {
    Hit h;
    float ox = x, oy = y + eyeH(), oz = z, cp = std::cos(pitch);
    float dx = -std::sin(yaw) * cp, dy = std::sin(pitch), dz = std::cos(yaw) * cp;
    int ix = (int)std::floor(ox), iy = (int)std::floor(oy), iz = (int)std::floor(oz);
    int sx = dx > 0 ? 1 : -1, sy = dy > 0 ? 1 : -1, sz = dz > 0 ? 1 : -1;
    const float big = 1e30f;
    auto td = [&](float p, float d, int s) { return d == 0 ? big : (s > 0 ? (std::floor(p) + 1 - p) : (p - std::floor(p))) / std::fabs(d); };
    auto dd = [&](float d) { return d == 0 ? big : std::fabs(1 / d); };
    float tx = td(ox, dx, sx), ty = td(oy, dy, sy), tz = td(oz, dz, sz), ax = dd(dx), ay = dd(dy), az = dd(dz), t = 0;
    int nx = 0, ny = 0, nz = 0;
    for (int i = 0; i < 40; i++) {
      int b = w.at(ix, iy, iz);
      if (b && (binfo(b).solid || isPartial(b) || (fluids && (b == WATER || b == LAVA)))) { h.ok = true; h.x = ix; h.y = iy; h.z = iz; h.nx = nx; h.ny = ny; h.nz = nz; h.b = b; return h; }
      if (tx < ty && tx < tz) { t = tx; ix += sx; tx += ax; nx = -sx; ny = nz = 0; }
      else if (ty < tz) { t = ty; iy += sy; ty += ay; ny = -sy; nx = nz = 0; }
      else { t = tz; iz += sz; tz += az; nz = -sz; nx = ny = 0; }
      if (t > maxd) break;
    }
    return h;
  }

  bool canHarvest(int b) {
    const BInfo& bi = binfo(b);
    if (bi.req == 0) return true;
    Stack& s = held();
    return !s.empty() && isTool(s.id) && toolType(s.id) == bi.tool && TLEVEL[toolTier(s.id)] >= bi.req;
  }
  float mineTime(int b) {
    if (mode == CREATIVE) return .15f;
    const BInfo& bi = binfo(b);
    float speed = 1;
    Stack& s = held();
    if (!s.empty() && isTool(s.id) && toolType(s.id) == bi.tool) speed = TSPEED[toolTier(s.id)];
    return bi.hard * (canHarvest(b) ? 1.5f : 5.f) / speed;
  }
  float mineProgress(const Hit& h) const { return mActive && h.ok && h.x == mx && h.y == my && h.z == mz ? mprog : 0; }
  Furn* furnAt(int fx, int fy, int fz) {
    for (auto& f : furn) if (f.x == fx && f.y == fy && f.z == fz && f.dim == w.dim) return &f;
    return nullptr;
  }
  bool mineStep(float dt, bool held_, const Hit& h) {
    if (locked || dead || !h.ok || !held_) { mprog = 0; mActive = false; return false; }
    if (!mActive || h.x != mx || h.y != my || h.z != mz) { mx = h.x; my = h.y; mz = h.z; mprog = 0; mActive = true; }
    mprog += dt;
    if (mprog < mineTime(h.b)) return false;
    mprog = 0;
    breakBlock(h);
    return true;
  }
  void breakBlock(const Hit& h) {
    int b = h.b;
    const BInfo& bi = binfo(b);
    bool creative = mode == CREATIVE;
    if (!creative) {
      if (canHarvest(b)) {
        int d = bi.drop == -1 ? b : bi.drop;
        if (b == GRAVEL && hash3(w.seed, h.x, h.y, h.z) < .12f) d = I_FLINT;
        if (d > 0) inv.add(d, 1);
        if (b == LEAVES && hash3(w.seed, h.x, h.y, h.z) < .08f) inv.add(I_APPLE, 1);
        if (b == FURNACE) {
          Furn* f = furnAt(h.x, h.y, h.z);
          if (f) {
            for (Stack* s : {&f->in, &f->fuel, &f->out}) if (!s->empty()) inv.add(s->id, s->n);
            furn.erase(furn.begin() + (f - &furn[0]));
          }
        }
      }
      Stack& s = held();
      if (!s.empty() && isTool(s.id) && bi.hard > 0) { if (--s.dur <= 0) s = Stack(); }
      addExh(.005f);
    }
    if (b == CHEST) {
      ChestData* c = chestAt(h.x, h.y, h.z, false, false);
      if (c) { for (auto& st_ : c->s) if (!st_.empty()) giveOrDrop(h.x + .5f, h.y + .5f, h.z + .5f, st_); chests.erase(chests.begin() + (c - &chests[0])); }
    }
    w.set(h.x, h.y, h.z, AIR);
    emit(EV_BREAK);
    if (b == OBSIDIAN) collapsePortals(h.x, h.y, h.z);
    int ab = w.at(h.x, h.y + 1, h.z);
    if (needsSupport(ab)) {  // things standing on the removed block fall off
      w.set(h.x, h.y + 1, h.z, AIR);
      if (mode != CREATIVE) inv.add(binfo(ab).drop == -1 ? ab : binfo(ab).drop, 1);
      circuitUpdate(h.x, h.y + 1, h.z);
    }
    if (isCircuit(b)) { doorsOpen.erase(World::pk(h.x, h.y, h.z)); circuitUpdate(h.x, h.y, h.z); }
  }
  // 0 nothing, 1 placed, 2 open crafting table, 3 open furnace
  int use(const Hit& h) {
    if (locked || dead) return 0;
    { Stack& a = held();  // right-click armor = equip it (swaps with what you wear)
      if (!a.empty() && isArmor(a.id)) { std::swap(armor[armorPiece(a.id)], a); return 4; } }
    if (!h.ok) return 0;
    if (!sneak) {  // levers, buttons, doors
      if (h.b == LEVER || h.b == LEVER_ON) { w.set(h.x, h.y, h.z, h.b == LEVER ? LEVER_ON : LEVER); circuitUpdate(h.x, h.y, h.z); emit(EV_CLICK); return 7; }
      if (h.b == BUTTON) { w.set(h.x, h.y, h.z, BUTTON_ON); timers.push_back({h.x, h.y, h.z, 1.0f}); circuitUpdate(h.x, h.y, h.z); emit(EV_CLICK); return 7; }
      if (h.b == DOOR || h.b == DOOR_OPEN) { w.set(h.x, h.y, h.z, h.b == DOOR ? DOOR_OPEN : DOOR); doorsOpen.erase(World::pk(h.x, h.y, h.z)); emit(EV_DOOR); return 7; }
      if (h.b == CHEST) return 6;
    }
    { Stack& f = held();  // flint and steel on obsidian lights a nether portal
      if (!f.empty() && f.id == I_FLINT_STEEL) {
        if (h.b != OBSIDIAN) return 0;
        static const int D[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
        for (auto& d : D) if (w.at(h.x + d[0], h.y + d[1], h.z + d[2]) == AIR && lightPortal(h.x + d[0], h.y + d[1], h.z + d[2])) {
          if (mode != CREATIVE && --f.dur <= 0) f = Stack();
          return 5;
        }
        return 0;
      } }
    if (h.b == TABLE && !sneak) return 2;
    if (h.b == FURNACE && !sneak) return 3;
    Stack& s = held();
    if (s.empty() || s.id >= BLK_COUNT) return 0;
    int px = h.x + h.nx, py = h.y + h.ny, pz = h.z + h.nz;
    if (py < 0 || py >= WH || binfo(w.at(px, py, pz)).solid) return 0;
    if (needsSupport(s.id) && !binfo(w.at(px, py - 1, pz)).solid) return 0;
    if (binfo(s.id).solid && px + 1 > x - .3f && px < x + .3f && py + 1 > y && py < y + 1.8f && pz + 1 > z - .3f && pz < z + .3f) return 0;
    w.set(px, py, pz, (uint8_t)s.id);
    emit(EV_PLACE);
    if (isCircuit(s.id)) circuitUpdate(px, py, pz);
    if (s.id == CHEST) chestAt(px, py, pz, true, false);
    if (s.id == FURNACE) { Furn f; f.x = px; f.y = py; f.z = pz; f.dim = w.dim; furn.push_back(f); }
    if (mode != CREATIVE) { if (--s.n <= 0) s = Stack(); }
    return 1;
  }
  // Hold to eat. Returns true on finishing a bite.
  bool eatStep(float dt, bool holding) {
    Stack& s = held();
    if (!holding || locked || dead || s.empty() || foodValue(s.id) == 0 || (v.food >= 20 && mode != CREATIVE)) { eatT = 0; return false; }
    eatT += dt;
    if (eatT < 1.6f) return false;
    eatT = 0;
    v.food = std::min(20.f, v.food + foodValue(s.id));
    emit(EV_EAT);
    v.sat = std::min(v.food, v.sat + foodValue(s.id) * .6f);
    if (mode != CREATIVE && --s.n <= 0) s = Stack();
    return true;
  }

  // ---------- armor, damage ----------
  int armorPoints() const { int p = 0; for (auto& a : armor) if (!a.empty()) p += ARMOR_DEF[armorMat(a.id)][armorPiece(a.id)]; return p; }
  void hurtMob(float dmg) {  // damage from monsters/arrows/explosions: reduced by armor, which wears down
    if (mode == CREATIVE || locked || dead || v.hurtT > 0) return;
    float red = std::min(.8f, armorPoints() * .04f);
    for (auto& a : armor) if (!a.empty() && --a.dur <= 0) a = Stack();
    float mult = 1;
    if (blocking && !held().empty() && held().id == I_SHIELD) { mult = .15f; Stack& sh = held(); if (--sh.dur <= 0) sh = Stack(); emit(EV_CLICK); }
    hurt(dmg * (1 - red) * mult);
  }
  void knockPlayer(float ux, float uz, float s) { vx += ux * s; vz += uz * s; vy = std::max(vy, 4.f); ground = false; }
  void dropItem(float px, float py, float pz, int id, int n) {
    int left = inv.add(id, n);
    emit(EV_PICKUP);
    if (left > 0) { Pile p; p.x = px; p.y = py; p.z = pz; p.dim = w.dim; Stack r = mk(id, left); p.items.push_back(r); piles.push_back(p); }
  }

  // ---------- fluids / world tick ----------
  void worldTick(float dt) {
    waterT += dt; lavaT += dt;
    bool wt = false, lv = false;
    if (waterT >= .25f) { waterT -= .25f; wt = true; }
    if (lavaT >= 1.5f) { lavaT -= 1.5f; lv = true; }
    if (wt || lv) w.fluidTick(wt, lv, 300);
    atkT -= dt; bowT -= dt; noteT -= dt;
    circuitTick(dt); updWeather(dt); updEyes(dt);
    updMobs(dt); updArrows(dt); spawnTick(dt);
  }
  // ---------- chests ----------
  ChestData* chestAt(int cx, int cy, int cz, bool create, bool loot) {
    for (auto& c : chests) if (c.x == cx && c.y == cy && c.z == cz && c.dim == w.dim) return &c;
    if (!create) return nullptr;
    ChestData c; c.x = cx; c.y = cy; c.z = cz; c.dim = w.dim;
    if (loot) {  // chests found in villages hold a little deterministic loot
      float r = hash3(w.seed, cx, cy, cz);
      c.s[0] = mk(I_COAL, 2 + (int)(hash3(w.seed, cx, cy + 1, cz) * 4));
      c.s[1] = mk(I_APPLE, 1 + (int)(hash3(w.seed, cx, cy + 2, cz) * 3));
      if (r < .5f) c.s[2] = mk(I_IRON, 1 + (int)(hash3(w.seed, cx, cy + 3, cz) * 3));
      if (r < .25f) c.s[3] = mk(I_EMERALD, 1 + (int)(hash3(w.seed, cx, cy + 4, cz) * 2));
    }
    chests.push_back(c);
    return &chests.back();
  }
  void giveOrDrop(float px, float py, float pz, const Stack& k) {
    int left = inv.add(k.id, k.n, k.dur);
    if (left > 0) { Pile p; p.x = px; p.y = py; p.z = pz; p.dim = w.dim; Stack r = k; r.n = left; p.items.push_back(r); piles.push_back(p); }
  }

  // ---------- circuits: lever / button / plate -> wire -> lamp / door ----------
  // Only the connected circuit around a change is recalculated (capped), never the whole world.
  void circuitUpdate(int sx, int sy, int sz) {
    static const int D[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    std::vector<std::array<int, 3>> cells, q;
    std::unordered_set<uint64_t> seen;
    auto push = [&](int x, int y, int z) {
      if (y < 0 || y >= WH || !isCircuit(w.at(x, y, z))) return;
      if (seen.insert(World::pk(x, y, z)).second) q.push_back({x, y, z});
    };
    push(sx, sy, sz);
    for (auto& d : D) push(sx + d[0], sy + d[1], sz + d[2]);
    while (!q.empty() && cells.size() < 700) {
      auto c = q.back(); q.pop_back(); cells.push_back(c);
      for (auto& d : D) push(c[0] + d[0], c[1] + d[1], c[2] + d[2]);
    }
    std::unordered_map<uint64_t, int> lv;
    std::vector<std::array<int, 3>> fr;
    for (auto& c : cells) if (circuitSource(w.at(c[0], c[1], c[2]))) { lv[World::pk(c[0], c[1], c[2])] = 15; fr.push_back(c); }
    for (size_t i = 0; i < fr.size(); i++) {  // power decays by 1 per wire
      auto c = fr[i];
      int L = lv[World::pk(c[0], c[1], c[2])];
      if (L <= 1) continue;
      for (auto& d : D) {
        int a = c[0] + d[0], b = c[1] + d[1], e = c[2] + d[2];
        uint8_t t = w.at(a, b, e);
        if (t == WIRE || t == WIRE_ON) { uint64_t k = World::pk(a, b, e); auto it = lv.find(k); if (it == lv.end() || it->second < L - 1) { lv[k] = L - 1; fr.push_back({a, b, e}); } }
      }
    }
    for (auto& c : cells) {
      int x = c[0], y = c[1], z = c[2];
      uint8_t b = w.at(x, y, z);
      if (b == WIRE || b == WIRE_ON) {
        auto it = lv.find(World::pk(x, y, z));
        uint8_t nb = (it != lv.end() && it->second > 0) ? WIRE_ON : WIRE;
        if (nb != b) w.set(x, y, z, nb);
      } else if (b == LAMP || b == LAMP_ON || b == DOOR || b == DOOR_OPEN) {
        bool pw = false;
        for (auto& d : D) {
          uint8_t n = w.at(x + d[0], y + d[1], z + d[2]);
          if (circuitSource(n) || ((n == WIRE || n == WIRE_ON) && lv.count(World::pk(x + d[0], y + d[1], z + d[2])))) pw = true;
        }
        if (b == LAMP || b == LAMP_ON) { uint8_t nb = pw ? LAMP_ON : LAMP; if (nb != b) w.set(x, y, z, nb); }
        else {
          uint64_t k = World::pk(x, y, z);
          bool was = doorsOpen.count(k) > 0;
          if (pw && !was) { doorsOpen.insert(k); w.set(x, y, z, DOOR_OPEN); emit(EV_DOOR); }
          else if (!pw && was) { doorsOpen.erase(k); w.set(x, y, z, DOOR); emit(EV_DOOR); }
        }
      }
    }
  }
  void circuitTick(float dt) {
    for (size_t i = 0; i < timers.size();) {  // buttons pop back out
      timers[i].t -= dt;
      if (timers[i].t <= 0) {
        BtnTimer b = timers[i];
        timers.erase(timers.begin() + i);
        if (w.at(b.x, b.y, b.z) == BUTTON_ON) { w.set(b.x, b.y, b.z, BUTTON); circuitUpdate(b.x, b.y, b.z); }
      } else i++;
    }
    if (dead || locked) return;
    int cx = (int)std::floor(x), cy = (int)std::floor(y + .05f), cz = (int)std::floor(z);  // pressure plates
    if (plateOn && (cx != plateX || cy != plateY || cz != plateZ)) {
      if (w.at(plateX, plateY, plateZ) == PLATE_ON) { w.set(plateX, plateY, plateZ, PLATE); circuitUpdate(plateX, plateY, plateZ); }
      plateOn = false;
    }
    if (!plateOn && w.at(cx, cy, cz) == PLATE) {
      w.set(cx, cy, cz, PLATE_ON); plateX = cx; plateY = cy; plateZ = cz; plateOn = true; circuitUpdate(cx, cy, cz); emit(EV_CLICK);
    }
  }
  bool litNear(int x, int y, int z) {  // torches, lamps and glowstone keep monsters from spawning nearby
    for (int a = -6; a <= 6; a++) for (int c = -6; c <= 6; c++) for (int b = -3; b <= 4; b++) {
      uint8_t t = w.at(x + a, y + b, z + c);
      if (t == TORCH || t == LAMP_ON || t == GLOWSTONE) return true;
    }
    return false;
  }

  // ---------- weather ----------
  int precip(float px, float pz) {  // 0 none, 1 rain, 2 snow (by biome)
    if (w.dim != OVERWORLD) return 0;
    int bx = (int)std::floor(px), bz = (int)std::floor(pz), bi = biomeAt(w.seed, bx, bz, terrainH(w.seed, bx, bz));
    return bi == DESERT ? 0 : bi == SNOWY ? 2 : 1;
  }
  void updWeather(float dt) {
    if (w.dim != OVERWORLD) { weather = 0; return; }
    weatherT -= dt; flash = std::max(0.f, flash - dt * 2); strikeShow = std::max(0.f, strikeShow - dt);
    if (weatherT <= 0) {
      float r = rnd();
      if (weather == 0) { weather = r < .6f ? 1 : r < .8f ? 2 : 0; weatherT = weather == 0 ? 200 + rnd() * 300 : 90 + rnd() * 150; }
      else if (weather == 1 && r < .4f) { weather = 2; weatherT = 60 + rnd() * 90; }  // rain can build into a thunderstorm
      else { weather = 0; weatherT = 240 + rnd() * 400; }
      strikeT = 6 + rnd() * 6;
    }
    if (weather == 2) {  // thunderstorm: lightning near the player
      strikeT -= dt;
      if (strikeT <= 0) {
        strikeT = 8 + rnd() * 14;
        float a = rnd() * 6.28318f, r = 5 + rnd() * 35;
        strikeX = x + std::cos(a) * r; strikeZ = z + std::sin(a) * r; strikeShow = .6f; flash = 1; emit(EV_THUNDER);
        if (std::hypot(strikeX - x, strikeZ - z) < 4) hurt(8);
      }
    }
  }

  // bucket: pick up a source block, or pour one out. Pass a ray hit made with fluids = true.
  int useBucket(const Hit& h) {
    if (locked || dead || !h.ok) return 0;
    Stack& s = held();
    if (s.empty()) return 0;
    if (s.id == I_BUCKET) {
      if ((h.b == WATER || h.b == LAVA) && w.level(h.x, h.y, h.z) == 8) {
        w.removeFluid(h.x, h.y, h.z);
        s = mk(h.b == WATER ? I_WBUCKET : I_LBUCKET, 1);
        return 1;
      }
      return 0;
    }
    if (s.id == I_WBUCKET || s.id == I_LBUCKET) {
      int px = h.x, py = h.y, pz = h.z;
      if (h.b != WATER && h.b != LAVA) { px += h.nx; py += h.ny; pz += h.nz; }
      uint8_t cur = w.at(px, py, pz);
      if (py < 0 || py >= WH || binfo(cur).solid) return 0;
      w.putFluid(px, py, pz, s.id == I_WBUCKET ? WATER : LAVA, 8);
      if (mode != CREATIVE) s = mk(I_BUCKET, 1);
      return 1;
    }
    return 0;
  }

  // ---------- nether portals ----------
  bool lightPortal(int ax_, int ay_, int az_) {  // (ax_,ay_,az_) = an air cell next to the obsidian that was clicked
    for (int axis = 0; axis < 2; axis++) {
      int dx = axis == 0 ? 1 : 0, dz = axis == 0 ? 0 : 1;
      int yb = ay_;
      while (yb > 0 && w.at(ax_, yb - 1, az_) == AIR && ay_ - yb < 22) yb--;
      if (w.at(ax_, yb - 1, az_) != OBSIDIAN) continue;
      int lx = ax_, lz = az_, cnt = 0;
      while (w.at(lx - dx, yb, lz - dz) == AIR && cnt < 22) { lx -= dx; lz -= dz; cnt++; }
      if (w.at(lx - dx, yb, lz - dz) != OBSIDIAN) continue;
      int wd = 1;
      while (w.at(lx + dx * wd, yb, lz + dz * wd) == AIR && wd < 22) wd++;
      if (w.at(lx + dx * wd, yb, lz + dz * wd) != OBSIDIAN || wd < 2) continue;
      int hh = 1;
      while (w.at(lx, yb + hh, lz) == AIR && hh < 22) hh++;
      if (w.at(lx, yb + hh, lz) != OBSIDIAN || hh < 3) continue;
      bool ok = true;
      for (int i = 0; i < wd && ok; i++) {
        if (w.at(lx + dx * i, yb - 1, lz + dz * i) != OBSIDIAN || w.at(lx + dx * i, yb + hh, lz + dz * i) != OBSIDIAN) ok = false;
        for (int j = 0; j < hh && ok; j++) if (w.at(lx + dx * i, yb + j, lz + dz * i) != AIR) ok = false;
      }
      for (int j = 0; j < hh && ok; j++) if (w.at(lx - dx, yb + j, lz - dz) != OBSIDIAN || w.at(lx + dx * wd, yb + j, lz + dz * wd) != OBSIDIAN) ok = false;
      if (!ok) continue;
      for (int i = 0; i < wd; i++) for (int j = 0; j < hh; j++) w.set(lx + dx * i, yb + j, lz + dz * i, PORTAL);
      return true;
    }
    return false;
  }
  void collapsePortals(int x, int y, int z) {  // breaking the frame removes the portal
    static const int D[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    std::vector<std::array<int, 3>> st;
    for (auto& d : D) if (w.at(x + d[0], y + d[1], z + d[2]) == PORTAL) st.push_back({x + d[0], y + d[1], z + d[2]});
    while (!st.empty()) {
      auto c = st.back(); st.pop_back();
      if (w.at(c[0], c[1], c[2]) != PORTAL) continue;
      w.set(c[0], c[1], c[2], AIR);
      for (auto& d : D) if (w.at(c[0] + d[0], c[1] + d[1], c[2] + d[2]) == PORTAL) st.push_back({c[0] + d[0], c[1] + d[1], c[2] + d[2]});
    }
  }
  void buildPortal(int px, int py, int pz) {  // obsidian frame in the X-Y plane, 2x3 opening, with a safe platform
    for (int a = -1; a <= 2; a++) for (int b = -1; b <= 3; b++) for (int c = -1; c <= 1; c++) w.set(px + a, py + b, pz + c, AIR);
    for (int a = -1; a <= 2; a++) for (int c = -1; c <= 1; c++) w.set(px + a, py - 1, pz + c, OBSIDIAN);
    for (int b = 0; b <= 2; b++) { w.set(px - 1, py + b, pz, OBSIDIAN); w.set(px + 2, py + b, pz, OBSIDIAN); }
    for (int a = 0; a <= 1; a++) w.set(px + a, py + 3, pz, OBSIDIAN);
    for (int a = 0; a <= 1; a++) for (int b = 0; b <= 2; b++) w.set(px + a, py + b, pz, PORTAL);
  }
  void portalTick(float dt) {
    auto cell = [&](float yy) { return w.at((int)std::floor(x), (int)std::floor(yy), (int)std::floor(z)); };
    int pb = (cell(y + .5f) == PORTAL || cell(y + 1.2f) == PORTAL) ? (int)PORTAL : (cell(y + .2f) == END_PORTAL || cell(y + .9f) == END_PORTAL) ? (int)END_PORTAL : 0;
    if (!pb) { portalT = 0; portalCool = false; return; }
    if (portalCool) return;
    portalT += dt;
    float need = pb == END_PORTAL ? 1.f : (mode == CREATIVE ? .6f : 3.5f);
    if (portalT >= need) { portalT = 0; portalCool = true; if (pb == PORTAL) travelPortal(); else travelEnd(); }
  }
  void travelPortal() {
    emit(EV_PORTAL);
    int from = w.dim, to = from == OVERWORLD ? NETHER : from == NETHER ? OVERWORLD : from;
    if (to == from) return;
    float sc = from == OVERWORLD ? 1 / 8.f : 8.f;
    float tx = x * sc, tz = z * sc;
    changeDim(to);
    int bx = (int)std::floor(tx), bz = (int)std::floor(tz), fx = 0, fy = 0, fz = 0;
    float best = 1e9f;
    for (int dx = -16; dx <= 16; dx++) for (int dz = -16; dz <= 16; dz++) for (int yy = 1; yy < WH - 1; yy++)
      if (w.at(bx + dx, yy, bz + dz) == PORTAL) { float d = (float)(dx * dx + dz * dz + (yy - 30) * (yy - 30) / 4); if (d < best) { best = d; fx = bx + dx; fy = yy; fz = bz + dz; } }
    if (best >= 1e9f) {  // none nearby: build a new one
      int by = to == NETHER ? 32 : std::max(terrainH(w.seed, bx, bz) + 1, SEA + 1);
      buildPortal(bx, by, bz);
      x = bx + 1.0f; y = (float)by + .01f; z = bz + 1.5f;
    } else {  // stand next to the existing portal, on solid ground
      bool placed = false;
      for (int r = 1; r <= 3 && !placed; r++)
        for (int dx = -r; dx <= r && !placed; dx++) for (int dz = -r; dz <= r && !placed; dz++)
          for (int dy = -1; dy <= 1 && !placed; dy++) {
            int a = fx + dx, b = fy + dy, c = fz + dz;
            if (w.at(a, b, c) == AIR && w.at(a, b + 1, c) == AIR && binfo(w.at(a, b - 1, c)).solid) { x = a + .5f; y = (float)b + .01f; z = c + .5f; placed = true; }
          }
      if (!placed) { x = fx + .5f; y = (float)fy + .01f; z = fz + .5f; }
    }
    vx = vy = vz = 0; fallPeak = y;
  }

  // ---------- mobs ----------
  bool mobHit(const Mob& m, float px, float py, float pz) {
    const MobInfo& I = minfo(m.type);
    float r = I.w / 2;
    int x0 = (int)std::floor(px - r), x1 = (int)std::floor(px + r), y0 = (int)std::floor(py), y1 = (int)std::floor(py + I.h - .01f), z0 = (int)std::floor(pz - r), z1 = (int)std::floor(pz + r);
    for (int a = x0; a <= x1; a++) for (int b = y0; b <= y1; b++) for (int c = z0; c <= z1; c++) if (binfo(w.at(a, b, c)).solid) return true;
    return false;
  }
  void moveMob(Mob& m, float dt) {
    bool wetm = w.at((int)std::floor(m.x), (int)std::floor(m.y + .3f), (int)std::floor(m.z)) == WATER;
    if (wetm) m.vy = std::min(1.5f, m.vy + 10 * dt); else m.vy = std::max(-30.f, m.vy - 26 * dt);
    float dxm = m.vx * dt; m.x += dxm;
    if (mobHit(m, m.x, m.y, m.z)) { m.x -= dxm; if (m.ground) m.vy = 8.f; m.vx = 0; }  // hop up ledges
    float dzm = m.vz * dt; m.z += dzm;
    if (mobHit(m, m.x, m.y, m.z)) { m.z -= dzm; if (m.ground) m.vy = 8.f; m.vz = 0; }
    m.ground = false;
    float dym = m.vy * dt;
    int n = std::max(1, (int)std::ceil(std::fabs(dym) / .4f));
    for (int i = 0; i < n; i++) {
      m.y += dym / n;
      if (mobHit(m, m.x, m.y, m.z)) { m.y -= dym / n; if (m.vy < 0) m.ground = true; m.vy = 0; break; }
    }
    if (m.y < -20) m.hp = 0;
  }
  void shootAt(const Mob& m) {
    float ox = m.x, oy = m.y + 1.4f, oz = m.z, dx = x - ox, dy = (y + 1.1f) - oy, dz = z - oz, d = std::sqrt(dx * dx + dy * dy + dz * dz) + 1e-4f;
    Arrow a; a.x = ox; a.y = oy; a.z = oz; a.vx = dx / d * 24; a.vz = dz / d * 24; a.vy = dy / d * 24 + d * .35f; a.dmg = 3; a.mine = false;
    arrows.push_back(a);
  }
  void explode(float ex, float ey, float ez, float r) {
    emit(EV_EXPLODE);
    int R = (int)std::ceil(r);
    for (int a = -R; a <= R; a++) for (int b = -R; b <= R; b++) for (int c = -R; c <= R; c++) {
      if (a * a + b * b + c * c > r * r) continue;
      int bx = (int)std::floor(ex) + a, by = (int)std::floor(ey) + b, bz = (int)std::floor(ez) + c;
      uint8_t t = w.at(bx, by, bz);
      if (t == AIR || t == BEDROCK || t == OBSIDIAN || t == WATER || t == LAVA || t == PORTAL) continue;
      if (hash3(w.seed, bx, by, bz) < .85f) w.set(bx, by, bz, AIR);
    }
    float d = std::sqrt((x - ex) * (x - ex) + (y + 1 - ey) * (y + 1 - ey) + (z - ez) * (z - ez));
    if (d < r * 2) { float ux = (x - ex) / (d + 1e-3f), uz = (z - ez) / (d + 1e-3f); hurtMob((1 - d / (r * 2)) * 18 * (diff == 3 ? 1.3f : 1.f)); knockPlayer(ux, uz, 9); }
    for (auto& m : mobs) { float md = std::sqrt((m.x - ex) * (m.x - ex) + (m.y - ey) * (m.y - ey) + (m.z - ez) * (m.z - ez)); if (md < r * 2) m.hp -= (1 - md / (r * 2)) * 18; }
  }
  void mobDrops(const Mob& m) {
    auto d = [&](int id, int lo, int hi) { int n = lo + (int)(rnd() * (hi - lo + 1)); if (n > 0) dropItem(m.x, m.y, m.z, id, n); };
    switch (m.type) {
      case M_COW: d(I_RAW_BEEF, 1, 3); d(I_LEATHER, 0, 2); break;
      case M_PIG: d(I_RAW_PORK, 1, 3); break;
      case M_SHEEP: d(I_WOOL, 1, 2); d(I_RAW_BEEF, 0, 1); break;
      case M_CHICKEN: d(I_RAW_CHICKEN, 1, 1); d(I_FEATHER, 0, 2); break;
      case M_ZOMBIE: d(I_ROTTEN, 0, 2); break;
      case M_SKELETON: d(I_BONE, 0, 2); d(I_ARROW, 0, 2); break;
      case M_SPIDER: d(I_STRING, 0, 2); break;
      case M_CREEPER: d(I_GUNPOWDER, 0, 2); break;
      case M_CINDER: d(I_GOLD, 0, 1); d(I_EMBER, 0, 2); break;
      case M_WARPER: d(I_PEARL, 0, 1); break;
      default: break;
    }
  }
  void updMobs(float dt) {
    std::vector<std::array<float, 3>> booms;
    for (size_t i = 0; i < mobs.size();) {
      Mob& m = mobs[i];
      const MobInfo& I = minfo(m.type);
      if (m.type == M_DRAGON) { updDragon(m, dt); if (m.hp <= 0) { dragonDefeated(); mobs.erase(mobs.begin() + i); continue; } i++; continue; }
      if (m.type == M_CRYSTAL) { m.anim += dt; m.hurt -= dt; if (m.hp <= 0) { booms.push_back({m.x, m.y, m.z}); mobs.erase(mobs.begin() + i); continue; } i++; continue; }
      m.atk -= dt; m.hurt -= dt; m.flee -= dt; m.ai -= dt;
      m.anim += std::hypot(m.vx, m.vz) * dt;
      float dx = x - m.x, dz = z - m.z, dist = std::hypot(dx, dz), dy = (y + .9f) - (m.y + I.h / 2);
      bool tgt = I.hostile && !(mode == CREATIVE || locked || dead) && diff > 0;
      float wx = 0, wz = 0, spd = I.speed;
      bool boom = false;
      if (m.type == M_WARPER && tgt && dist < 30) {  // blinks around, especially when hit
        m.t1 -= dt;
        if (m.t1 <= 0 || (m.hurt > .3f && rnd() < .3f)) { m.t1 = 4 + rnd() * 4; teleportMob(m); }
      }
      if (tgt && dist < 20 && std::fabs(dy) < 9) {
        float ux = dx / (dist + 1e-4f), uz = dz / (dist + 1e-4f);
        if (m.type == M_SKELETON) {
          if (dist > 9) { wx = ux; wz = uz; } else if (dist < 5) { wx = -ux; wz = -uz; }
          if (dist < 15 && m.atk <= 0 && std::fabs(dy) < 4) { shootAt(m); m.atk = 2.2f; }
        } else if (m.type == M_CREEPER) {
          if (m.fuse >= 0) { m.fuse -= dt; if (dist > 6.f) m.fuse = -1; else if (m.fuse <= 0) boom = true; }
          else { wx = ux; wz = uz; if (dist < 2.4f) m.fuse = 1.5f; }
        } else {
          wx = ux; wz = uz;
          if (dist < I.w / 2 + .9f && std::fabs(dy) < 1.6f && m.atk <= 0) {
            m.atk = 1.0f;
            hurtMob(I.dmg * (diff == 3 ? 1.5f : diff == 1 ? .6f : 1.f));
            knockPlayer(ux, uz, 6);
          }
        }
      } else if (m.flee > 0 && !I.hostile) { wx = -dx / (dist + 1e-4f); wz = -dz / (dist + 1e-4f); spd *= 1.8f; }
      else {  // wander
        if (m.ai <= 0) {
          m.ai = 2 + rnd() * 4;
          if (rnd() < .5f) { m.tx = m.x; m.tz = m.z; } else { m.tx = m.x + (rnd() - .5f) * 12; m.tz = m.z + (rnd() - .5f) * 12; }
          if (m.type == M_VILLAGER && std::hypot(m.tx - m.hx, m.tz - m.hz) > 9) { m.tx = m.hx; m.tz = m.hz; }  // villagers stay near home
        }
        float ex = m.tx - m.x, ez = m.tz - m.z, ed = std::hypot(ex, ez);
        if (ed > .6f) { wx = ex / ed; wz = ez / ed; spd *= .6f; }
      }
      float k = std::min(1.f, dt * 8);
      m.vx += (wx * spd - m.vx) * k; m.vz += (wz * spd - m.vz) * k;
      if (wx != 0 || wz != 0) m.yaw = std::atan2(-wx, wz);
      moveMob(m, dt);
      if (boom) { booms.push_back({m.x, m.y + .5f, m.z}); mobs.erase(mobs.begin() + i); continue; }
      if (m.hp <= 0) { Mob dead_ = m; mobs.erase(mobs.begin() + i); mobDrops(dead_); continue; }
      i++;
    }
    for (auto& b : booms) explode(b[0], b[1], b[2], 3.2f);
  }
  void updArrows(float dt) {
    for (size_t i = 0; i < arrows.size();) {
      Arrow& a = arrows[i];
      bool gone = false;
      a.life -= dt; a.vy -= 18 * dt;
      float sp = std::sqrt(a.vx * a.vx + a.vy * a.vy + a.vz * a.vz);
      int n = std::max(1, (int)std::ceil(sp * dt / .4f));
      for (int s = 0; s < n && !gone; s++) {
        a.x += a.vx * dt / n; a.y += a.vy * dt / n; a.z += a.vz * dt / n;
        if (binfo(w.at((int)std::floor(a.x), (int)std::floor(a.y), (int)std::floor(a.z))).solid) { gone = true; break; }
        if (!a.mine) {
          if (!dead && !locked && mode != CREATIVE && std::fabs(a.x - x) < .45f && a.y > y && a.y < y + 1.9f && std::fabs(a.z - z) < .45f) {
            hurtMob(a.dmg); knockPlayer(a.vx / (sp + 1e-3f), a.vz / (sp + 1e-3f), 3); gone = true;
          }
        } else {
          for (auto& m : mobs) {
            const MobInfo& I = minfo(m.type);
            if (std::fabs(a.x - m.x) < I.w / 2 + .2f && a.y > m.y - .1f && a.y < m.y + I.h + .1f && std::fabs(a.z - m.z) < I.w / 2 + .2f) {
              m.hp -= a.dmg; m.hurt = .4f; m.flee = 4; m.vx += a.vx / (sp + 1e-3f) * 4; m.vz += a.vz / (sp + 1e-3f) * 4; m.vy = 3; gone = true; break;
            }
          }
        }
      }
      if (gone || a.life <= 0) arrows.erase(arrows.begin() + i); else i++;
    }
  }
  bool skyOpen(int bx, int by, int bz) { for (int yy = by + 1; yy < WH; yy++) if (binfo(w.at(bx, yy, bz)).solid) return false; return true; }
  void spawnTick(float dt) {
    spawnT -= dt;
    if (spawnT > 0) return;
    spawnT = 1.5f;
    int nh = 0, np = 0;
    for (size_t i = 0; i < mobs.size();) {
      Mob& m = mobs[i];
      float d = std::hypot(m.x - x, m.z - z);
      bool hos = minfo(m.type).hostile && m.type != M_DRAGON, kill = (d > 72 && m.type != M_DRAGON && m.type != M_CRYSTAL) || (hos && diff == 0);
      if (!kill && hos && w.dim == OVERWORLD && daylight() > .7f && d > 24 && rnd() < .08f && skyOpen((int)std::floor(m.x), (int)std::floor(m.y), (int)std::floor(m.z))) kill = true;
      if (kill) mobs.erase(mobs.begin() + i); else { if (hos) nh++; else if (m.type <= M_CHICKEN) np++; i++; }
    }
    if (dead || locked) return;
    if (w.dim == OVERWORLD) {  // villagers live in villages (respawned from the world seed, so no persistence needed)
      int prx = (int)std::floor(x / 64.f), prz = (int)std::floor(z / 64.f);
      for (int a = -1; a <= 1; a++) for (int b = -1; b <= 1; b++) {
        VillageInfo v = villageIn(w.seed, prx + a, prz + b);
        if (!v.ok || std::hypot(v.cx - x, v.cz - z) > 56) continue;
        int nearv = 0;
        for (auto& m : mobs) if (m.type == M_VILLAGER && std::hypot(m.x - v.cx, m.z - v.cz) < 26) nearv++;
        for (int k = 0; k < 4 && nearv < 4; k++) {
          float vx_ = v.cx + VH_X[k] + .5f, vz_ = v.cz + VH_Z[k] + .5f;
          if (w.at((int)std::floor(vx_), v.y0 + 1, (int)std::floor(vz_)) != AIR) continue;
          bool taken = false;
          for (auto& m : mobs) if (m.type == M_VILLAGER && std::hypot(m.x - vx_, m.z - vz_) < 4) taken = true;
          if (taken) continue;
          Mob m; m.type = M_VILLAGER; m.x = vx_; m.y = (float)v.y0 + 1; m.z = vz_; m.hp = 20; m.hx = vx_; m.hz = vz_;
          m.prof = (int)(hash3(w.seed, v.cx, k, v.cz) * 6);
          mobs.push_back(m); nearv++;
        }
      }
    }
    for (int tr = 0; tr < 4; tr++) {
      float a = rnd() * 6.2831853f, r = 18 + rnd() * 22;
      int sx = (int)std::floor(x + std::cos(a) * r), sz = (int)std::floor(z + std::sin(a) * r), sy = -1;
      for (int yy = std::min(WH - 3, (int)y + 12); yy >= std::max(1, (int)y - 16); yy--) {
        uint8_t b = w.at(sx, yy - 1, sz);
        if (binfo(b).solid && b != LEAVES && w.at(sx, yy, sz) == AIR && w.at(sx, yy + 1, sz) == AIR) { sy = yy; break; }
      }
      if (sy < 0) continue;
      uint8_t ground_ = w.at(sx, sy - 1, sz);
      bool open = skyOpen(sx, sy, sz), dark = w.dim != OVERWORLD || daylight() < .35f || (!open && sy < terrainH(w.seed, sx, sz) - 1);  // night, or underground
      int type = -1;
      if (dark && w.dim != END && !litNear(sx, sy, sz) && diff > 0 && nh < (mode == HARDCORE ? 16 : 10) && rnd() < .6f) {
        if (w.dim == NETHER) { float q = rnd(); type = q < .7f ? M_CINDER : q < .85f ? M_SKELETON : M_WARPER; }
        else { float q = rnd(); type = q < .4f ? M_ZOMBIE : q < .65f ? M_SKELETON : q < .85f ? M_SPIDER : q < .93f ? M_CREEPER : M_WARPER; }
      } else if (w.dim == END && diff > 0 && nh < 5 && rnd() < .4f) {
        type = M_WARPER;
      } else if (w.dim == OVERWORLD && np < 10 && daylight() > .4f && (ground_ == GRASS || ground_ == SNOW) && open) type = (int)(rnd() * 4);
      if (type < 0) continue;
      Mob m; m.type = type; m.x = sx + .5f; m.y = (float)sy; m.z = sz + .5f; m.hp = minfo(type).hp; m.yaw = rnd() * 6.28f;
      mobs.push_back(m);
      if (minfo(type).hostile) nh++; else np++;
    }
  }

  // ---------- villagers ----------
  bool doTrade(int mi, int ti) {
    if (mi < 0 || mi >= (int)mobs.size() || mobs[mi].type != M_VILLAGER) return false;
    Mob& m = mobs[mi];
    const auto& T = trades(m.prof);
    if (ti < 0 || ti >= (int)T.size() || m.uses[ti] >= T[ti].max || inv.count(T[ti].give) < T[ti].giveN) return false;
    Inv bak = inv;
    inv.take(T[ti].give, T[ti].giveN);
    if (inv.add(T[ti].get, T[ti].getN) > 0) { inv = bak; return false; }
    m.uses[ti]++;
    emit(EV_CRAFT);
    return true;
  }
  int villagerTarget() { int i = targetMob(); return (i >= 0 && mobs[i].type == M_VILLAGER) ? i : -1; }

  // ---------- End progression: seeker eyes, End portal, Warper, dragon ----------
  bool nearestStronghold(float px, float pz, float& bx, float& bz) {
    if (w.dim != OVERWORLD) return false;
    float best = 1e18f;
    bool ok = false;
    for (int i = 0; i < 3; i++) {
      int sx, sz;
      strongholdPos(w.seed, i, sx, sz);
      float d = (sx - px) * (sx - px) + (sz - pz) * (sz - pz);
      if (d < best) { best = d; bx = (float)sx; bz = (float)sz; ok = true; }
    }
    return ok;
  }
  void checkEndPortal(int fx, int fy, int fz) {
    for (int cx = fx - 3; cx <= fx + 3; cx++) for (int cz = fz - 3; cz <= fz + 3; cz++) {
      bool ok = true;
      for (int a = -2; a <= 2 && ok; a++) for (int b = -2; b <= 2 && ok; b++) {
        bool ring = std::max(std::abs(a), std::abs(b)) == 2 && !(std::abs(a) == 2 && std::abs(b) == 2);
        if (ring && w.at(cx + a, fy, cz + b) != END_FRAME_EYE) ok = false;
      }
      if (!ok) continue;
      for (int a = -1; a <= 1; a++) for (int b = -1; b <= 1; b++) w.set(cx + a, fy, cz + b, END_PORTAL);
      say("The portal to the End opens!", 5);
      emit(EV_PORTAL);
      return;
    }
  }
  // Seeker Eye: click an empty portal frame to fill it, or use it in the air to see which way the nearest stronghold lies.
  int useEye(const Hit& h) {
    if (locked || dead) return 0;
    Stack& s = held();
    if (s.empty() || s.id != I_EYE) return 0;
    if (h.ok && h.b == END_FRAME) {
      w.set(h.x, h.y, h.z, END_FRAME_EYE);
      if (mode != CREATIVE && --s.n <= 0) s = Stack();
      emit(EV_CLICK);
      checkEndPortal(h.x, h.y, h.z);
      return 1;
    }
    float bx, bz;
    if (!nearestStronghold(x, z, bx, bz)) { say("The eye does not react here."); return 0; }
    float dx = bx - x, dz = bz - z, d = std::hypot(dx, dz) + 1e-3f;
    eyes.push_back({x, y + 1.6f, z, dx / d, dz / d, 0});
    if (mode != CREATIVE && --s.n <= 0) s = Stack();
    say("The Seeker Eye drifts toward a stronghold about " + std::to_string((int)(d / 10) * 10) + " blocks away", 5);
    return 2;
  }
  void updEyes(float dt) {
    for (size_t i = 0; i < eyes.size();) {
      auto& e = eyes[i];
      e.t += dt; e.x += e.dx * 5 * dt; e.z += e.dz * 5 * dt; e.y += (e.t < 1.2f ? 2.4f : -1.2f) * dt;
      if (e.t >= 2.5f) {
        if (rnd() < .8f) { Pile p; p.x = e.x; p.y = e.y; p.z = e.z; p.dim = w.dim; p.items.push_back(mk(I_EYE, 1)); piles.push_back(p); }  // 20% shatter
        eyes.erase(eyes.begin() + i);
      } else i++;
    }
  }
  void travelEnd() {
    emit(EV_PORTAL);
    if (w.dim == END) { dposSet[OVERWORLD] = false; changeDim(OVERWORLD); say(victory ? "Welcome home, hero." : "Back in the Overworld.", 4); }
    else changeDim(END);
  }
  void spawnDragon() {
    Mob d; d.type = M_DRAGON; d.x = 0; d.y = 50; d.z = 0; d.hp = minfo(M_DRAGON).hp;
    mobs.push_back(d);
    for (int i = 0; i < 8; i++) {
      float a = i * 0.785398f;
      Mob c; c.type = M_CRYSTAL; c.x = std::lround(std::cos(a) * 20) + .5f; c.z = std::lround(std::sin(a) * 20) + .5f; c.y = 32.f + 6 + (i % 3) * 3 + 1; c.hp = 4;
      mobs.push_back(c);
    }
    say("The Sky Wyrm guards the End. Destroy the crystals that heal it!", 6);
  }
  void teleportMob(Mob& m) {
    for (int tr = 0; tr < 6; tr++) {
      float a = rnd() * 6.28318f, r = 4 + rnd() * 8;
      int tx = (int)std::floor(x + std::cos(a) * r), tz = (int)std::floor(z + std::sin(a) * r);
      for (int yy = std::min(WH - 4, (int)y + 6); yy >= std::max(1, (int)y - 8); yy--)
        if (binfo(w.at(tx, yy - 1, tz)).solid && w.at(tx, yy, tz) == AIR && w.at(tx, yy + 1, tz) == AIR && w.at(tx, yy + 2, tz) == AIR) { m.x = tx + .5f; m.y = (float)yy; m.z = tz + .5f; m.vx = m.vz = 0; return; }
    }
  }
  void updDragon(Mob& m, float dt) {
    m.anim += dt; m.hurt -= dt; m.atk -= dt; m.ai -= dt; m.t1 += dt;
    const float maxHp = minfo(M_DRAGON).hp;
    bool tgt = !(mode == CREATIVE || locked || dead) && diff > 0, enraged = m.hp <= maxHp * .5f;
    int crystals = 0;
    for (auto& q : mobs) if (q.type == M_CRYSTAL) crystals++;
    if (crystals > 0 && m.hp < maxHp) m.hp = std::min(maxHp, m.hp + 2 * dt);  // crystals heal it
    if (m.t2 <= 0 && m.ai <= 0 && tgt) { m.t2 = 4.f; m.ai = 9 + rnd() * 4; emit(EV_DRAGON); }  // swoop at the player now and then
    float tx, ty, tz, spd;
    if (m.t2 > 0 && tgt) { tx = x; ty = y + 1; tz = z; spd = enraged ? 20.f : 15.f; m.t2 -= dt; }
    else { m.t2 = 0; float a = m.t1 * (enraged ? .55f : .4f); tx = std::cos(a) * 26; tz = std::sin(a) * 26; ty = 46 + std::sin(m.t1 * .7f) * 4; spd = 12; }
    float dx = tx - m.x, dy = ty - m.y, dz = tz - m.z, d = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (d > .05f) { float st = std::min(d, spd * dt); m.x += dx / d * st; m.y += dy / d * st; m.z += dz / d * st; m.yaw = std::atan2(-dx, dz); }
    float px = x - m.x, py = (y + 1) - m.y, pz = z - m.z, pd = std::sqrt(px * px + py * py + pz * pz);
    if (tgt && pd < 3.4f && m.atk <= 0) { m.atk = 1.2f; m.t2 = 0; hurtMob(8 * (diff == 3 ? 1.3f : 1.f)); knockPlayer(px / (pd + 1e-3f), pz / (pd + 1e-3f), 10); }
    if (tgt && enraged && m.t2 <= 0 && m.atk <= 0 && pd < 50) { shootAt(m); m.atk = 2.5f; }  // breath attack
  }
  void dragonDefeated() {
    dragonDead = true; victory = true;
    for (int a = -1; a <= 1; a++) for (int b = -1; b <= 1; b++) w.set(a, 33, b, END_PORTAL);  // exit portal
    dropItem(.5f, 33, .5f, I_EMERALD, 12);
    dropItem(.5f, 33, .5f, I_DIAMOND, 4);
    say("YOU DEFEATED THE SKY WYRM! The exit portal has opened.", 10);
    emit(EV_EXPLODE);
  }

  // ---------- combat ----------
  int targetMob(float maxd = 3.4f) {
    float ox = x, oy = y + eyeH(), oz = z, cp = std::cos(pitch), dx = -std::sin(yaw) * cp, dy = std::sin(pitch), dz = std::cos(yaw) * cp;
    Hit hb = raycast(maxd);
    float limit = maxd;
    if (hb.ok) limit = std::sqrt((hb.x + .5f - ox) * (hb.x + .5f - ox) + (hb.y + .5f - oy) * (hb.y + .5f - oy) + (hb.z + .5f - oz) * (hb.z + .5f - oz)) - .3f;
    int best = -1;
    float bt = 1e9f;
    for (size_t i = 0; i < mobs.size(); i++) {
      const Mob& m = mobs[i];
      const MobInfo& I = minfo(m.type);
      float cx = m.x - ox, cy = m.y + I.h / 2 - oy, cz = m.z - oz, t = cx * dx + cy * dy + cz * dz;
      if (t < 0 || t > maxd || t > limit + .5f) continue;
      float px = cx - dx * t, py = cy - dy * t, pz = cz - dz * t, rr = I.w / 2 + .25f, vr = I.h / 2 + .2f;
      if (px * px + pz * pz < rr * rr && std::fabs(py) < vr && t < bt) { bt = t; best = (int)i; }
    }
    return best;
  }
  bool attackMob(int i) {
    if (locked || dead || i < 0 || i >= (int)mobs.size()) return false;
    Stack& s = held();
    bool tool = !s.empty() && isTool(s.id);
    float dmg = tool ? (float)toolDamage(s.id) : 1.f, cdMax = (tool && toolType(s.id) == 3) ? .6f : (tool && toolType(s.id) == 1) ? 1.0f : .5f;
    float factor = atkT <= 0 ? 1.f : .35f;  // swinging again too early is weak
    bool crit = !ground && vy < 0 && !wet && factor > .9f;
    if (crit) dmg *= 1.5f;
    dmg *= factor;
    atkT = cdMax;
    if (tool && mode != CREATIVE && (s.dur -= (toolType(s.id) == 3 ? 1 : 2)) <= 0) s = Stack();
    Mob& m = mobs[i];
    emit(EV_ATTACK);
    m.hp -= dmg; m.hurt = .4f; m.flee = 4;
    float ux = m.x - x, uz = m.z - z, ul = std::hypot(ux, uz) + 1e-4f;
    m.vx = ux / ul * 7; m.vz = uz / ul * 7; m.vy = 5; m.ground = false;
    if (m.hp <= 0) { Mob d = m; mobs.erase(mobs.begin() + i); mobDrops(d); }
    return true;
  }
  bool shoot() {
    Stack& s = held();
    if (s.empty() || s.id != I_BOW || bowT > 0 || locked || dead) return false;
    if (mode != CREATIVE && inv.count(I_ARROW) == 0) return false;
    if (mode != CREATIVE) { inv.take(I_ARROW, 1); if (--s.dur <= 0) s = Stack(); }
    bowT = .7f;
    emit(EV_SHOOT);
    float cp = std::cos(pitch), dx = -std::sin(yaw) * cp, dy = std::sin(pitch), dz = std::cos(yaw) * cp;
    Arrow a; a.x = x + dx * .5f; a.y = y + eyeH() + dy * .5f; a.z = z + dz * .5f; a.vx = dx * 32; a.vy = dy * 32; a.vz = dz * 32; a.dmg = 6; a.mine = true;
    arrows.push_back(a);
    return true;
  }

  // ---------- save / load ----------
  std::string serialize() {
    std::ostringstream o;
    o.precision(9);
    o << "VOX1\nname " << name << "\nid " << id << "\nseed " << w.seed << "\nmode " << mode << "\ndiff " << diff << "\ndim " << w.dim
      << "\ndead " << (dead ? 1 : 0) << "\nlocked " << (locked ? 1 : 0) << "\nplayed " << played << "\n";
    o << "pos " << x << ' ' << y << ' ' << z << ' ' << yaw << ' ' << pitch << "\ntime " << time << "\nvit " << v.hp << ' ' << v.food << ' ' << v.sat << ' ' << v.exh << ' ' << v.air
      << "\nsel " << sel << "\ninv\n";
    for (auto& s : inv.s) o << s.id << ' ' << s.n << ' ' << s.dur << "\n";
    o << "armor\n";
    for (auto& s : armor) o << s.id << ' ' << s.n << ' ' << s.dur << "\n";
    o << "furn " << furn.size() << "\n";
    for (auto& f : furn)
      o << f.x << ' ' << f.y << ' ' << f.z << ' ' << f.dim << ' ' << f.in.id << ' ' << f.in.n << ' ' << f.in.dur << ' ' << f.fuel.id << ' ' << f.fuel.n << ' ' << f.fuel.dur << ' '
        << f.out.id << ' ' << f.out.n << ' ' << f.out.dur << ' ' << f.burn << ' ' << f.burnMax << ' ' << f.prog << "\n";
    o << "piles " << piles.size() << "\n";
    for (auto& p : piles) {
      o << p.x << ' ' << p.y << ' ' << p.z << ' ' << p.dim << ' ' << p.items.size() << "\n";
      for (auto& s : p.items) o << s.id << ' ' << s.n << ' ' << s.dur << "\n";
    }
    size_t ftotal = 0;
    for (int d = 0; d < 3; d++) ftotal += w.fl[d].size();
    o << "fluids " << ftotal << "\n";
    for (int d = 0; d < 3; d++) for (auto& e : w.fl[d]) { int fx, fy, fz; World::unpk(e.first, fx, fy, fz); o << d << ' ' << fx << ' ' << fy << ' ' << fz << ' ' << (int)e.second << "\n"; }
    o << "chests " << chests.size() << "\n";
    for (auto& c : chests) { o << c.x << ' ' << c.y << ' ' << c.z << ' ' << c.dim; for (auto& k : c.s) o << ' ' << k.id << ' ' << k.n << ' ' << k.dur; o << "\n"; }
    o << "endstate " << (dragonDead ? 1 : 0) << ' ' << (victory ? 1 : 0) << "\n";
    size_t total = 0;
    for (int d = 0; d < 3; d++) for (auto& c : w.edits[d]) total += c.second.size();
    o << "edits " << total << "\n";
    for (int d = 0; d < 3; d++) for (auto& c : w.edits[d]) for (auto& e : c.second) o << d << ' ' << ckx(c.first) << ' ' << ckz(c.first) << ' ' << e.first << ' ' << (int)e.second << "\n";
    return o.str();
  }
  bool deserialize(const std::string& txt) {
    std::istringstream in(txt);
    std::string l;
    if (!std::getline(in, l) || l != "VOX1") return false;
    auto hdr = [&](const char* key, std::string& out) {
      std::string ln;
      if (!std::getline(in, ln)) return false;
      size_t sp = ln.find(' ');
      if (ln.substr(0, sp) != key) return false;
      out = sp == std::string::npos ? "" : ln.substr(sp + 1);
      return true;
    };
    std::string a;
    *this = Game();
    try {
      if (!hdr("name", name) || !hdr("id", id)) return false;
      if (!hdr("seed", a)) return false;
      w.seed = (uint32_t)std::stoll(a);
      if (!hdr("mode", a)) return false;
      mode = std::stoi(a);
      if (!hdr("diff", a)) return false;
      diff = std::stoi(a);
      if (!hdr("dim", a)) return false;
      w.dim = std::stoi(a);
      if (!hdr("dead", a)) return false;
      dead = a == "1";
      if (!hdr("locked", a)) return false;
      locked = a == "1";
      if (!hdr("played", a)) return false;
      played = std::stoll(a);
    } catch (...) { return false; }
    std::string tag;
    in >> tag >> x >> y >> z >> yaw >> pitch;
    in >> tag >> time;
    in >> tag >> v.hp >> v.food >> v.sat >> v.exh >> v.air;
    in >> tag >> sel;
    in >> tag;
    for (auto& s : inv.s) in >> s.id >> s.n >> s.dur;
    in >> tag;
    if (tag == "armor") { for (auto& s : armor) in >> s.id >> s.n >> s.dur; in >> tag; }
    size_t n = 0;
    in >> n;  // "furn" count
    for (size_t i = 0; i < n && in; i++) {
      Furn f;
      in >> f.x >> f.y >> f.z >> f.dim >> f.in.id >> f.in.n >> f.in.dur >> f.fuel.id >> f.fuel.n >> f.fuel.dur >> f.out.id >> f.out.n >> f.out.dur >> f.burn >> f.burnMax >> f.prog;
      furn.push_back(f);
    }
    in >> tag >> n;  // "piles"
    for (size_t i = 0; i < n && in; i++) {
      Pile p; size_t k = 0;
      in >> p.x >> p.y >> p.z >> p.dim >> k;
      for (size_t j = 0; j < k; j++) { Stack s; in >> s.id >> s.n >> s.dur; p.items.push_back(s); }
      piles.push_back(p);
    }
    in >> tag;
    if (tag == "fluids") {
      in >> n;
      for (size_t i = 0; i < n && in; i++) { int d, fx, fy, fz, lv; in >> d >> fx >> fy >> fz >> lv; if (d >= 0 && d < 3) w.fl[d][World::pk(fx, fy, fz)] = (uint8_t)lv; }
      in >> tag;
    }
    if (tag == "chests") {
      in >> n;
      for (size_t i = 0; i < n && in; i++) { ChestData c; in >> c.x >> c.y >> c.z >> c.dim; for (auto& k : c.s) in >> k.id >> k.n >> k.dur; chests.push_back(c); }
      in >> tag;
    }
    if (tag == "endstate") { int a = 0, b = 0; in >> a >> b; dragonDead = a; victory = b; in >> tag; }
    in >> n;  // "edits" count
    for (size_t i = 0; i < n && in; i++) {
      int d, cx, cz, ix_, b;
      in >> d >> cx >> cz >> ix_ >> b;
      if (d >= 0 && d < 3) w.edits[d][ckey(cx, cz)][ix_] = (uint8_t)b;
    }
    if (!in && n > 0 && in.fail() && !in.eof()) return false;
    sel = std::max(0, std::min(8, sel));
    fallPeak = y;
    return true;
  }
};

// ---------- persistence ----------
inline std::string& saveRoot() { static std::string r = "saves"; return r; }
inline std::string saveDir() { return saveRoot(); }
inline bool saveGame(Game& g) {
  std::error_code ec;
  std::filesystem::create_directories(saveDir(), ec);
  std::ofstream f(saveDir() + "/" + g.id + ".sav", std::ios::binary | std::ios::trunc);
  if (!f) return false;
  f << g.serialize();
  return (bool)f;
}
inline bool loadGame(const std::string& id, Game& g) {
  std::ifstream f(saveDir() + "/" + id + ".sav", std::ios::binary);
  if (!f) return false;
  std::stringstream ss; ss << f.rdbuf();
  return g.deserialize(ss.str());
}
inline void deleteWorld(const std::string& id) {
  std::error_code ec;
  std::filesystem::remove(saveDir() + "/" + id + ".sav", ec);
}
inline std::vector<Meta> listWorlds() {
  std::vector<Meta> out;
  std::error_code ec;
  if (!std::filesystem::exists(saveDir(), ec)) return out;
  for (auto& e : std::filesystem::directory_iterator(saveDir(), ec)) {
    if (e.path().extension() != ".sav") continue;
    std::ifstream f(e.path(), std::ios::binary);
    std::string l, ver;
    if (!std::getline(f, ver) || ver != "VOX1") continue;
    Meta m;
    auto rd = [&]() { std::getline(f, l); size_t sp = l.find(' '); return sp == std::string::npos ? std::string() : l.substr(sp + 1); };
    try {
      m.name = rd(); m.id = rd(); m.seed = std::stoll(rd()); m.mode = std::stoi(rd()); rd(); m.dim = std::stoi(rd());
      m.dead = rd() == "1"; rd(); m.played = std::stoll(rd());
    } catch (...) { continue; }
    out.push_back(m);
  }
  std::sort(out.begin(), out.end(), [](const Meta& a, const Meta& b) { return a.played > b.played; });
  return out;
}

struct Settings {
  int rd = 1;  // 0 Low, 1 Medium, 2 High, 3 Ultra
  bool vibrant = true, bob = true;
  float fov = 75, sens = 1, tsens = 1, vol = .7f;
  int skin = 0;
  std::string ser() const {
    std::ostringstream o;
    o << rd << ' ' << vibrant << ' ' << bob << ' ' << fov << ' ' << sens << ' ' << tsens << ' ' << skin << ' ' << vol;
    return o.str();
  }
  void parse(const std::string& s) {
    std::istringstream i(s);
    Settings t;
    int v = 0, b = 0;
    if (i >> t.rd >> v >> b >> t.fov >> t.sens >> t.tsens >> t.skin) {
      float vv = .7f;
      if (i >> vv) t.vol = std::max(0.f, std::min(1.f, vv));
      t.vibrant = v; t.bob = b;
      t.rd = std::max(0, std::min(3, t.rd)); t.fov = std::max(50.f, std::min(110.f, t.fov));
      t.sens = std::max(.2f, std::min(3.f, t.sens)); t.tsens = std::max(.2f, std::min(3.f, t.tsens)); t.skin = std::max(0, std::min(5, t.skin));
      *this = t;
    }
  }
};
inline int rdChunks(int i) { static const int v[4] = {3, 5, 7, 10}; return v[std::max(0, std::min(3, i))]; }
}  // namespace vx
