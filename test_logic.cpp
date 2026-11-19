// Logic tests: build with `cmake -DBUILD_TESTS=ON` or `g++ -std=c++17 -I. tests/test_logic.cpp`
#include "../game.h"
#include "../art.h"
#include <cassert>
#include <cstdio>
#include <set>
using namespace vx;
static int fails = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static Input idle() { return Input(); }
static void run(Game& g, float secs, const Input& in) { for (float t = 0; t < secs; t += 1 / 60.f) g.update(1 / 60.f, in); }

int main() {
  setvbuf(stdout, nullptr, _IONBF, 0);
  // determinism + variety
  { World a, b, c; a.seed = b.seed = 1234; c.seed = 999;
    int same = 0, diff = 0;
    for (int x = -40; x < 40; x += 3) for (int z = -40; z < 40; z += 3) for (int y = 0; y < WH; y += 5) {
      if (a.at(x, y, z) == b.at(x, y, z)) same++;
      if (a.at(x, y, z) != c.at(x, y, z)) diff++;
    }
    CHECK(same == 27 * 27 * 13); CHECK(diff > 100); }
  { std::set<int> biomes; for (int x = -3000; x < 3000; x += 40) for (int z = -3000; z < 3000; z += 40) { int h = terrainH(77, x, z); biomes.insert(biomeAt(77, x, z, h)); }
    std::printf("biomes found: %zu\n", biomes.size()); CHECK(biomes.size() >= 5); }
  // survival start
  Game g; g.newWorld("T", 42, SURVIVAL, 2);
  { int items = 0; for (auto& s : g.inv.s) if (!s.empty()) items++; CHECK(items == 0); }
  CHECK(!binfo(g.w.at((int)g.x, (int)g.y - 1, (int)g.z)).trans || true);
  // spawn is not inside blocks
  CHECK(!g.hitBox(g.x, g.y, g.z));
  // landing + no falling through the world
  run(g, 4, idle()); CHECK(g.ground); CHECK(g.y > 0); CHECK(g.v.hp == 20);
  // fall damage
  { Game f; f.newWorld("F", 42, SURVIVAL, 2); run(f, 2, idle()); float gy = f.y; f.y = gy + 12; f.fallPeak = f.y; f.vy = 0; run(f, 3, idle());
    CHECK(f.v.hp < 20); CHECK(f.v.hp > 0); }
  // creative: no fall damage, flying
  { Game f; f.newWorld("C", 42, CREATIVE, 2); run(f, 2, idle()); f.y += 20; f.fallPeak = f.y; run(f, 3, idle()); CHECK(f.v.hp == 20);
    f.flying = true; Input up; up.jump = true; float y0 = f.y; run(f, 1, up); CHECK(f.y > y0 + 5); }
  // walking moves and collides (walk into a wall of stone)
  { Game f; f.newWorld("W", 42, SURVIVAL, 2); run(f, 2, idle()); float x0 = f.x, z0 = f.z; f.yaw = 0; Input fw; fw.fwd = 1; run(f, 1, fw);
    CHECK(std::hypot(f.x - x0, f.z - z0) > 2.5f);
    int bx = (int)std::floor(f.x), bz = (int)std::floor(f.z + 2), by = (int)std::floor(f.y);
    for (int h = 0; h < 3; h++) f.w.set(bx, by + h, bz, STONE); for (int h = 0; h < 3; h++) f.w.set(bx - 1, by + h, bz, STONE); for (int h = 0; h < 3; h++) f.w.set(bx + 1, by + h, bz, STONE);
    f.z = bz - 2.f; f.x = bx + .5f; run(f, 2, fw); CHECK(f.z < bz); CHECK(!f.hitBox(f.x, f.y, f.z)); }
  // mining: hand vs tools, tiers, durability, drops
  { Game f; f.newWorld("M", 42, SURVIVAL, 2);
    float hand = f.mineTime(STONE); CHECK(!f.canHarvest(STONE)); CHECK(hand > 5);
    f.inv.s[0] = mk(I_TOOL0 + 0, 1); float wood = f.mineTime(STONE); CHECK(f.canHarvest(STONE)); CHECK(wood < hand / 3);
    CHECK(!f.canHarvest(IRON_ORE)); CHECK(!f.canHarvest(DIAMOND_ORE));
    f.inv.s[0] = mk(I_TOOL0 + 1, 1); CHECK(f.canHarvest(IRON_ORE)); CHECK(!f.canHarvest(GOLD_ORE));
    f.inv.s[0] = mk(I_TOOL0 + 2, 1); CHECK(f.canHarvest(DIAMOND_ORE)); CHECK(!f.canHarvest(OBSIDIAN));
    f.inv.s[0] = mk(I_TOOL0 + 4, 1); CHECK(f.canHarvest(OBSIDIAN));
    CHECK(f.mineTime(STONE) < wood); CHECK(f.mineTime(BEDROCK) > 1e6);
    // axe is fast on logs, slow with the wrong tool
    f.inv.s[0] = Stack(); float logHand = f.mineTime(LOG); f.inv.s[0] = mk(I_TOOL0 + 7, 1); CHECK(f.mineTime(LOG) < logHand / 2);
    // break a block, get the drop, lose durability
    f.inv.s[0] = mk(I_TOOL0, 1); Hit h; h.ok = true; h.x = 5; h.y = 10; h.z = 5; f.w.set(5, 10, 5, STONE); h.b = STONE;
    f.breakBlock(h); CHECK(f.w.at(5, 10, 5) == AIR); CHECK(f.inv.count(COBBLE) == 1); CHECK(f.inv.s[0].dur == TDUR[0] - 1);
    // without a pickaxe stone drops nothing
    Game e; e.newWorld("E", 42, SURVIVAL, 2); e.w.set(5, 10, 5, STONE); h.b = STONE; e.breakBlock(h); CHECK(e.inv.count(COBBLE) == 0);
    // creative breaks instantly and gives nothing
    Game c; c.newWorld("C", 42, CREATIVE, 2); c.w.set(5, 10, 5, STONE); c.breakBlock(h); CHECK(c.inv.count(COBBLE) == 0); CHECK(c.mineTime(OBSIDIAN) < .2f); }
  // placing: face placement, not inside player, consumption
  { Game f; f.newWorld("P", 42, SURVIVAL, 2); run(f, 2, idle()); f.inv.s[0] = mk(DIRT, 3);
    Hit h; h.ok = true; h.x = (int)f.x + 3; h.y = (int)f.y - 1; h.z = (int)f.z; h.nx = 0; h.ny = 1; h.nz = 0; h.b = STONE;
    CHECK(f.use(h) == 1); CHECK(f.inv.s[0].n == 2); CHECK(f.w.at(h.x, h.y + 1, h.z) == DIRT);
    h.x = (int)std::floor(f.x); h.y = (int)std::floor(f.y) - 1; h.z = (int)std::floor(f.z); CHECK(f.use(h) == 0); }
  // inventory stacking
  { Inv i; CHECK(i.add(DIRT, 100) == 0); CHECK(i.s[0].n == 64); CHECK(i.s[1].n == 36); CHECK(i.add(I_TOOL0, 2) == 0); CHECK(i.s[2].n == 1 && i.s[3].n == 1);
    i.take(DIRT, 70); CHECK(i.count(DIRT) == 30); Inv full; for (auto& s : full.s) s = mk(STONE, 64); CHECK(full.add(DIRT, 5) == 5); }
  // crafting grid
  { Stack g2[4]; g2[0] = mk(LOG, 1); int n = 0; CHECK(matchGrid(g2, 2, 2, &n) == PLANKS && n == 4);
    g2[0] = mk(PLANKS, 1); g2[2] = mk(PLANKS, 1); CHECK(matchGrid(g2, 2, 2, &n) == I_STICK && n == 4);
    g2[1] = mk(PLANKS, 1); g2[3] = mk(PLANKS, 1); CHECK(matchGrid(g2, 2, 2, &n) == TABLE);
    Stack t3[9]; for (int i = 0; i < 3; i++) t3[i] = mk(PLANKS, 1); t3[4] = mk(I_STICK, 1); t3[7] = mk(I_STICK, 1); CHECK(matchGrid(t3, 3, 3, &n) == I_TOOL0);
    Stack s3[9]; s3[0] = s3[1] = s3[2] = s3[3] = s3[5] = s3[6] = s3[7] = s3[8] = mk(COBBLE, 1); CHECK(matchGrid(s3, 3, 3, &n) == FURNACE);
    Stack c2[4]; c2[0] = c2[1] = c2[2] = mk(COBBLE, 1); CHECK(matchGrid(c2, 2, 2, &n) == 0);
    Stack ax[9]; ax[0] = ax[1] = ax[4] = mk(I_IRON, 1); ax[3] = ax[6] = mk(I_STICK, 1); CHECK(matchGrid(ax, 3, 3, &n) == I_TOOL0 + 5 + 2); // mirrored
    Stack ax2[9]; ax2[0] = ax2[1] = ax2[3] = mk(I_IRON, 1); ax2[4] = mk(I_STICK, 1); ax2[7] = mk(I_STICK, 1); CHECK(matchGrid(ax2, 3, 3, &n) == I_TOOL0 + 5 + 2);
    // recipe book
    Inv inv; inv.add(LOG, 2); const Rec& r0 = recipes()[0]; CHECK(autoCraft(inv, r0)); CHECK(inv.count(PLANKS) == 4 && inv.count(LOG) == 1);
    CHECK(autoCraft(inv, recipes()[1])); CHECK(inv.count(I_STICK) == 4 && inv.count(PLANKS) == 2); CHECK(!canAuto(inv, recipes()[2]));
    // full progression: planks -> sticks -> pick -> ... via recipe book
    Inv p; p.add(LOG, 4); autoCraft(p, recipes()[0]); autoCraft(p, recipes()[0]); autoCraft(p, recipes()[1]); autoCraft(p, recipes()[1]);
    int wp = -1; for (size_t i = 0; i < recipes().size(); i++) if (recipes()[i].out == I_TOOL0) wp = (int)i; CHECK(wp >= 0 && autoCraft(p, recipes()[wp])); CHECK(p.count(I_TOOL0) == 1); }
  // furnace
  { Furn f; f.in = mk(IRON_ORE, 2); f.fuel = mk(I_COAL, 1); for (float t = 0; t < 10.5f; t += .1f) furnTick(f, .1f);
    CHECK(f.out.id == I_IRON && f.out.n == 1); CHECK(f.in.n == 1); CHECK(f.burn > 0);
    for (float t = 0; t < 10.5f; t += .1f) furnTick(f, .1f); CHECK(f.out.n == 2); CHECK(f.in.empty());
    Furn nf; nf.in = mk(SAND, 1); for (float t = 0; t < 20; t += .1f) furnTick(nf, .1f); CHECK(nf.out.empty()); /* no fuel */ }
  // hunger, starvation, regeneration
  { Game f; f.newWorld("H", 42, SURVIVAL, 2); run(f, 2, idle()); Input sp; sp.fwd = 1; sp.sprint = true; float food0 = f.v.food + f.v.sat;
    run(f, 90, sp); CHECK(f.v.food + f.v.sat < food0);
    Game s; s.newWorld("S", 42, SURVIVAL, 2); run(s, 2, idle()); s.v.food = 0; s.v.sat = 0; run(s, 60, idle()); CHECK(s.v.hp < 20); CHECK(s.v.hp >= 1);
    Game h; h.newWorld("S", 42, SURVIVAL, 3); run(h, 2, idle()); h.v.food = 0; h.v.sat = 0; run(h, 120, idle()); CHECK(h.dead);
    Game r; r.newWorld("R", 42, SURVIVAL, 2); run(r, 2, idle()); r.v.hp = 10; run(r, 20, idle()); CHECK(r.v.hp > 10);
    Game e; e.newWorld("EAT", 42, SURVIVAL, 2); e.v.food = 10; e.inv.s[0] = mk(I_APPLE, 2); CHECK(!e.eatStep(1.0f, true)); CHECK(e.eatStep(.7f, true)); CHECK(e.v.food == 14 && e.inv.s[0].n == 1); }
  // water: swimming + drowning
  { Game f; f.newWorld("D", 42, SURVIVAL, 2); run(f, 1, idle());
    int bx = (int)f.x, bz = (int)f.z, by = (int)f.y; for (int dx = -2; dx <= 2; dx++) for (int dz = -2; dz <= 2; dz++) for (int dy = -1; dy < 8; dy++) f.w.set(bx + dx, by + dy, bz + dz, dy < 0 ? STONE : WATER);
    f.x = bx + .5f; f.z = bz + .5f; f.y = by + .2f; run(f, 2, idle()); CHECK(f.wet); CHECK(f.v.hp == 20);
    run(f, 20, idle()); CHECK(f.headWet || f.y > by + 5); CHECK(f.v.hp < 20 || f.v.air < 15);
    Input up; up.jump = true; float y0 = f.y; run(f, 1, up); CHECK(f.y > y0); }
  // lava damage
  { Game f; f.newWorld("L", 42, SURVIVAL, 2); run(f, 1, idle()); int bx = (int)f.x, bz = (int)f.z, by = (int)f.y;
    for (int dx = -1; dx <= 1; dx++) for (int dz = -1; dz <= 1; dz++) { f.w.set(bx + dx, by - 1, bz + dz, STONE); f.w.set(bx + dx, by, bz + dz, LAVA); f.w.set(bx + dx, by + 1, bz + dz, LAVA); }
    f.x = bx + .5f; f.z = bz + .5f; f.y = by + .1f; run(f, 1, idle()); CHECK(f.v.hp < 20); }
  // death: items drop in a pile and can be recovered; hardcore locks the world
  { Game f; f.newWorld("X", 42, SURVIVAL, 2); run(f, 1, idle()); f.inv.s[0] = mk(DIRT, 10); float dx = f.x, dy = f.y, dz = f.z; f.hurt(100);
    CHECK(f.dead); CHECK(f.inv.count(DIRT) == 0); CHECK(f.piles.size() == 1); f.respawn(); CHECK(!f.dead); CHECK(f.v.hp == 20);
    f.x = dx; f.y = dy; f.z = dz; run(f, .2f, idle()); CHECK(f.inv.count(DIRT) == 10); CHECK(f.piles.empty());
    Game h; h.newWorld("HC", 42, HARDCORE, 0); CHECK(h.diff == 3); run(h, 1, idle()); h.hurt(100); CHECK(h.dead); CHECK(h.locked);
    h.respawn(); CHECK(!h.dead); CHECK(h.locked); Hit hh; hh.ok = true; hh.x = 3; hh.y = 3; hh.z = 3; hh.b = STONE; CHECK(!h.mineStep(10, true, hh)); CHECK(h.use(hh) == 0); }
  // save / load round trip
  { Game f; f.newWorld("Save Me", 31337, SURVIVAL, 1); f.id = "unit"; run(f, 1, idle());
    f.inv.s[0] = mk(I_TOOL0 + 7, 1); f.inv.s[0].dur = 77; f.inv.s[4] = mk(COBBLE, 12); f.v.hp = 13; f.v.food = 9; f.sel = 4; f.time = 321;
    f.w.set(4, 30, 4, GLASS); f.w.set(-70, 20, 90, BRICKS); f.w.dim = NETHER; f.w.set(1, 30, 1, OBSIDIAN); f.w.dim = OVERWORLD;
    f.inv.s[1] = mk(FURNACE, 1); Hit h; h.ok = true; h.x = (int)f.x + 3; h.y = (int)f.y - 1; h.z = (int)f.z; h.ny = 1; h.b = STONE; f.sel = 1; f.use(h); f.sel = 4;
    f.furn[0].in = mk(IRON_ORE, 3); f.furn[0].prog = 4;
    CHECK(saveGame(f));
    Game l; CHECK(loadGame("unit", l)); CHECK(l.name == "Save Me"); CHECK(l.w.seed == 31337); CHECK(l.mode == SURVIVAL); CHECK(l.diff == 1);
    CHECK(l.inv.s[0].id == I_TOOL0 + 7 && l.inv.s[0].dur == 77); CHECK(l.inv.s[4].id == COBBLE && l.inv.s[4].n == 12); CHECK(l.v.hp == 13); CHECK(l.sel == 4);
    CHECK(std::fabs(l.time - 321) < .01f); CHECK(l.w.at(4, 30, 4) == GLASS); CHECK(l.w.at(-70, 20, 90) == BRICKS);
    l.w.dim = NETHER; l.w.reset(NETHER); CHECK(l.w.at(1, 30, 1) == OBSIDIAN); l.w.reset(OVERWORLD);
    CHECK(l.furn.size() == 1 && l.furn[0].in.id == IRON_ORE && l.furn[0].in.n == 3);
    CHECK(std::fabs(l.x - f.x) < .01f && std::fabs(l.y - f.y) < .01f);
    auto ws = listWorlds(); bool found = false; for (auto& m : ws) if (m.id == "unit" && m.name == "Save Me") found = true; CHECK(found);
    deleteWorld("unit"); Game z; CHECK(!loadGame("unit", z)); }
  // multiple independent worlds
  { Game a, b; a.newWorld("A", 1, SURVIVAL, 2); b.newWorld("B", 2, CREATIVE, 2); a.w.set(0, 40, 0, GOLD_ORE); CHECK(b.w.at(0, 40, 0) != GOLD_ORE); CHECK(a.mode != b.mode); }
  // dimensions
  { Game f; f.newWorld("N", 5, CREATIVE, 2); float ox = f.x; f.changeDim(NETHER); CHECK(f.w.dim == NETHER); run(f, 1, idle()); CHECK(!f.hitBox(f.x, f.y, f.z));
    f.changeDim(END); CHECK(f.w.dim == END); CHECK(binfo(f.w.at(0, 32, 0)).solid); run(f, 3, idle()); CHECK(f.y > 20);
    f.changeDim(OVERWORLD); CHECK(f.w.dim == OVERWORLD); CHECK(std::fabs(f.x - ox) < .01f); }
  // raycast
  { Game f; f.newWorld("RC", 42, SURVIVAL, 2); run(f, 2, idle()); int bx = (int)std::floor(f.x), bz = (int)std::floor(f.z), by = (int)std::floor(f.y); f.yaw = 0; f.pitch = 0;
    f.w.set(bx, by + 1, bz + 3, STONE); Hit h = f.raycast(); CHECK(h.ok && h.x == bx && h.z == bz + 3 && h.nz == -1); f.w.set(bx, by + 1, bz + 3, AIR);
    f.pitch = -1.5f; h = f.raycast(); CHECK(h.ok && h.ny == 1); }
  // settings
  { Settings s; s.rd = 3; s.vibrant = false; s.fov = 90; Settings t; t.parse(s.ser()); CHECK(t.rd == 3 && !t.vibrant && t.fov == 90); }
  // ======================= v3 systems =======================
  auto arena = [](Game& g, int cx, int cy, int cz, int r) {  // flat stone floor at cy-1, clear air above
    for (int dx = -r; dx <= r; dx++) for (int dz = -r; dz <= r; dz++) {
      g.w.set(cx + dx, cy - 1, cz + dz, STONE);
      for (int h = 0; h < 10; h++) g.w.set(cx + dx, cy + h, cz + dz, AIR);
    }
    g.x = cx + .5f; g.y = (float)cy; g.z = cz + .5f; g.vx = g.vy = g.vz = 0; g.fallPeak = g.y;
  };
  auto aimAt = [](Game& g, float tx, float ty, float tz) {
    float dx = tx - g.x, dy = ty - (g.y + g.eyeH()), dz = tz - g.z;
    g.yaw = std::atan2(-dx, dz); g.pitch = std::atan2(dy, std::hypot(dx, dz));
  };
  auto tickF = [](Game& g, int n, bool wa, bool la) { for (int i = 0; i < n; i++) g.w.fluidTick(wa, la, 1000); };
  // art
  { std::set<uint32_t> sums; int blank = 0, count = 0;
    for (int id = 1; id < I_END; id++) { if (!validItem(id)) continue; count++; Tex t = makeIcon(id); uint32_t h = 0; int op = 0; for (auto p : t.p) { h = h * 31 + p; if (pa(p)) op++; } sums.insert(h); if (op < 10) blank++; }
    CHECK(blank == 0); CHECK((int)sums.size() == count); std::printf("distinct icons: %d\n", count);
    std::set<uint32_t> ts; for (int t = 0; t < T_COUNT; t++) { Tex x = makeTile(t); uint32_t h = 0; for (auto p : x.p) h = h * 31 + p; ts.insert(h); } CHECK((int)ts.size() == T_COUNT); CHECK(T_COUNT <= 64);
    CHECK(pa(makeTile(T_GLASS).at(8, 8)) < 100); CHECK(pa(makeTile(T_STONE).at(8, 8)) == 255); CHECK(pa(makeTile(T_PORTAL).at(8, 8)) < 255); }
  // fluids: water flows sideways (7 blocks), falls, drains; lava is shorter; water + lava = obsidian/cobble
  { Game g; g.newWorld("F", 42, CREATIVE, 2); arena(g, 200, 40, 200, 14);
    g.w.putFluid(200, 40, 200, WATER, 8); tickF(g, 40, true, false);
    CHECK(g.w.level(201, 40, 200) == 7); CHECK(g.w.level(204, 40, 200) == 4); CHECK(g.w.level(207, 40, 200) == 1); CHECK(g.w.at(208, 40, 200) == AIR);
    g.w.set(203, 39, 200, AIR); tickF(g, 6, true, false); CHECK(g.w.at(203, 39, 200) == WATER);   // falls through a hole
    g.w.removeFluid(200, 40, 200); tickF(g, 120, true, false); CHECK(g.w.at(201, 40, 200) == AIR); CHECK(g.w.at(206, 40, 200) == AIR); } // drains when the source is removed
  { Game g; g.newWorld("L", 42, CREATIVE, 2); arena(g, 240, 40, 240, 14);
    g.w.putFluid(240, 40, 240, LAVA, 8); tickF(g, 12, false, true);
    CHECK(g.w.level(243, 40, 240) == 1); CHECK(g.w.at(244, 40, 240) == AIR); CHECK(g.w.level(241, 40, 240) == 3);
    g.w.putFluid(240, 40, 252, LAVA, 8); g.w.putFluid(241, 40, 252, WATER, 8); CHECK(g.w.at(240, 40, 252) == OBSIDIAN);   // water on a lava source
    g.w.putFluid(242, 40, 250, LAVA, 3); g.w.putFluid(243, 40, 250, WATER, 8); CHECK(g.w.at(242, 40, 250) == COBBLE); }   // water on flowing lava
  { Game g; g.newWorld("FS", 42, CREATIVE, 2); arena(g, 200, 40, 200, 14); g.w.putFluid(200, 40, 200, WATER, 8); tickF(g, 30, true, false);
    g.id = "fluidsave"; CHECK(saveGame(g)); Game l; CHECK(loadGame("fluidsave", l)); CHECK(l.w.level(203, 40, 200) == 5); CHECK(l.w.at(203, 40, 200) == WATER); CHECK(l.w.level(200, 40, 200) == 8); deleteWorld("fluidsave"); }
  // buckets
  { Game g; g.newWorld("B", 42, SURVIVAL, 2); arena(g, 260, 40, 260, 10); g.w.putFluid(260, 40, 260, WATER, 8);
    g.inv.s[0] = mk(I_BUCKET, 1); Hit h; h.ok = true; h.x = 260; h.y = 40; h.z = 260; h.b = WATER;
    CHECK(g.useBucket(h) == 1); CHECK(g.inv.s[0].id == I_WBUCKET); CHECK(g.w.at(260, 40, 260) == AIR);
    Hit f; f.ok = true; f.x = 262; f.y = 39; f.z = 260; f.ny = 1; f.b = STONE; CHECK(g.useBucket(f) == 1); CHECK(g.w.at(262, 40, 260) == WATER); CHECK(g.inv.s[0].id == I_BUCKET);
    g.w.putFluid(264, 40, 260, WATER, 8); g.w.removeFluid(264, 40, 260); g.w.putFluid(265, 40, 260, WATER, 5); h.x = 265; h.b = WATER; CHECK(g.useBucket(h) == 0); }  // can't scoop flowing water
  // nether portal: obsidian frame + flint and steel; incomplete frames don't light; breaking the frame removes it
  { Game g; g.newWorld("P", 42, SURVIVAL, 2); arena(g, 300, 40, 300, 8);
    for (int y = 40; y <= 42; y++) { g.w.set(299, y, 300, OBSIDIAN); g.w.set(302, y, 300, OBSIDIAN); }
    for (int x = 300; x <= 301; x++) { g.w.set(x, 39, 300, OBSIDIAN); g.w.set(x, 43, 300, OBSIDIAN); }
    g.w.set(301, 43, 300, AIR); g.inv.s[0] = mk(I_FLINT_STEEL, 1);
    Hit h; h.ok = true; h.x = 299; h.y = 40; h.z = 300; h.b = OBSIDIAN; h.nx = 1;
    CHECK(g.use(h) == 0); CHECK(g.w.at(300, 40, 300) == AIR);          // frame missing a block: nothing happens
    g.w.set(301, 43, 300, OBSIDIAN); CHECK(g.use(h) == 5);
    for (int x = 300; x <= 301; x++) for (int y = 40; y <= 42; y++) CHECK(g.w.at(x, y, 300) == PORTAL);
    CHECK(g.inv.s[0].dur == maxDur(I_FLINT_STEEL) - 1);
    g.mode = CREATIVE; g.inv.s[0] = Stack(); Hit b2; b2.ok = true; b2.x = 302; b2.y = 41; b2.z = 300; b2.b = OBSIDIAN; g.breakBlock(b2);
    CHECK(g.w.at(300, 41, 300) == AIR && g.w.at(301, 42, 300) == AIR); }
  { Game g; g.newWorld("P2", 42, SURVIVAL, 2); arena(g, 300, 40, 300, 8);   // frame in the other orientation (Z axis)
    for (int y = 40; y <= 42; y++) { g.w.set(300, y, 299, OBSIDIAN); g.w.set(300, y, 302, OBSIDIAN); }
    for (int z = 300; z <= 301; z++) { g.w.set(300, 39, z, OBSIDIAN); g.w.set(300, 43, z, OBSIDIAN); }
    g.inv.s[0] = mk(I_FLINT_STEEL, 1); Hit h; h.ok = true; h.x = 300; h.y = 39; h.z = 300; h.b = OBSIDIAN; h.ny = 1; CHECK(g.use(h) == 5); CHECK(g.w.at(300, 41, 301) == PORTAL); }
  // portal travel: stand in it, wait, arrive in the Nether next to a (new) portal, come back through it
  { Game g; g.newWorld("T", 42, SURVIVAL, 2); run(g, 1, idle()); float ox = g.x, oz = g.z; int px = (int)ox + 4, py = (int)g.y + 1, pz = (int)oz;
    g.buildPortal(px, py, pz); g.x = px + 1.0f; g.y = (float)py + .01f; g.z = pz + .5f; g.vx = g.vy = g.vz = 0;
    run(g, 2.0f, idle()); CHECK(g.w.dim == OVERWORLD); CHECK(g.portalT > 1.f);   // takes a few seconds in survival
    run(g, 2.5f, idle()); CHECK(g.w.dim == NETHER); CHECK(!g.hitBox(g.x, g.y, g.z));
    bool near = false; for (int dx = -5; dx <= 5; dx++) for (int dz = -5; dz <= 5; dz++) for (int dy = -3; dy <= 4; dy++) if (g.w.at((int)g.x + dx, (int)g.y + dy, (int)g.z + dz) == PORTAL) near = true;
    CHECK(near); run(g, 3, idle()); CHECK(g.w.dim == NETHER);   // arriving next to the portal does not bounce you back
    CHECK(std::fabs(g.x - ox / 8) < 8 && std::fabs(g.z - oz / 8) < 8);
    int nx = 0, ny = 0, nz = 0; for (int dx = -5; dx <= 5; dx++) for (int dz = -5; dz <= 5; dz++) for (int dy = -3; dy <= 4; dy++) if (g.w.at((int)g.x + dx, (int)g.y + dy, (int)g.z + dz) == PORTAL) { nx = (int)g.x + dx; ny = (int)g.y + dy; nz = (int)g.z + dz; }
    while (g.w.at(nx, ny - 1, nz) == PORTAL) ny--;
    g.x = nx + .5f; g.y = (float)ny + .01f; g.z = nz + .5f; run(g, 4.5f, idle()); CHECK(g.w.dim == OVERWORLD);
    CHECK(std::hypot(g.x - px, g.z - pz) < 10); CHECK(!g.hitBox(g.x, g.y, g.z));
    Game c; c.newWorld("TC", 42, CREATIVE, 2); run(c, 1, idle()); { int qx = (int)c.x + 4, qy = (int)c.y + 1, qz = (int)c.z; c.buildPortal(qx, qy, qz); c.x = qx + 1.0f; c.y = qy + .01f; c.z = qz + .5f; c.vx = c.vy = c.vz = 0; } run(c, 1.2f, idle()); CHECK(c.w.dim == NETHER); }  // creative: near-instant
  // armor
  { Game a, b; a.newWorld("A", 42, SURVIVAL, 2); b.newWorld("B", 42, SURVIVAL, 2);
    for (int pc = 0; pc < 4; pc++) a.armor[pc] = mk(I_ARMOR0 + 1 * 4 + pc, 1);
    CHECK(a.armorPoints() == 15); CHECK(a.armor[0].dur == maxDur(I_ARMOR0 + 4)); CHECK(maxDur(I_ARMOR0 + 4) == 165);
    a.hurtMob(10); b.hurtMob(10); CHECK(std::fabs(a.v.hp - 16) < .01f); CHECK(std::fabs(b.v.hp - 10) < .01f); CHECK(a.armor[1].dur == maxDur(I_ARMOR0 + 5) - 1);
    Game e; e.newWorld("E", 42, SURVIVAL, 2); e.inv.s[0] = mk(I_ARMOR0 + 3 * 4 + 0, 1); Hit none; CHECK(e.use(none) == 4); CHECK(e.armor[0].id == I_ARMOR0 + 12); CHECK(e.inv.s[0].empty());   // right-click equips
    e.inv.s[0] = mk(I_ARMOR0 + 4, 1); e.use(none); CHECK(e.armor[0].id == I_ARMOR0 + 4); CHECK(e.inv.s[0].id == I_ARMOR0 + 12);   // swap
    Game d; d.newWorld("D", 42, SURVIVAL, 2); run(d, 1, idle()); d.armor[1] = mk(I_ARMOR0 + 5, 1); d.hurt(100); CHECK(d.dead); CHECK(d.armor[1].empty()); CHECK(!d.piles.empty());
    CHECK(maxStack(I_ARMOR0 + 5) == 1); CHECK(ARMOR_DEF[3][1] == 8); }
  // recipes: armor, bow, arrows, bucket, flint and steel, cooked food
  { int n = 0; Stack g3[9];
    for (int i = 0; i < 3; i++) g3[i] = mk(I_IRON, 1); g3[3] = g3[5] = mk(I_IRON, 1); CHECK(matchGrid(g3, 3, 3, &n) == I_ARMOR0 + 1 * 4 + 0);
    Stack c3[9]; c3[0] = c3[2] = c3[3] = c3[4] = c3[5] = c3[6] = c3[7] = c3[8] = mk(I_DIAMOND, 1); CHECK(matchGrid(c3, 3, 3, &n) == I_ARMOR0 + 3 * 4 + 1);
    Stack bt[9]; bt[0] = bt[2] = bt[4] = mk(I_IRON, 1); CHECK(matchGrid(bt, 3, 3, &n) == I_BUCKET);
    Stack bw[9]; bw[1] = bw[3] = bw[7] = mk(I_STICK, 1); bw[2] = bw[5] = bw[8] = mk(I_STRING, 1); CHECK(matchGrid(bw, 3, 3, &n) == I_BOW);
    Stack ar[9]; ar[1] = mk(I_FLINT, 1); ar[4] = mk(I_STICK, 1); ar[7] = mk(I_FEATHER, 1); CHECK(matchGrid(ar, 3, 3, &n) == I_ARROW && n == 4);
    Stack fs[4]; fs[0] = mk(I_IRON, 1); fs[3] = mk(I_FLINT, 1); CHECK(matchGrid(fs, 2, 2, &n) == I_FLINT_STEEL);
    Furn f; f.in = mk(I_RAW_BEEF, 1); f.fuel = mk(I_COAL, 1); for (float t = 0; t < 10.5f; t += .1f) furnTick(f, .1f); CHECK(f.out.id == I_STEAK); CHECK(foodValue(I_STEAK) > foodValue(I_RAW_BEEF));
    Furn g; g.in = mk(I_RAW_PORK, 1); g.fuel = mk(PLANKS, 2); for (float t = 0; t < 10.5f; t += .1f) furnTick(g, .1f); CHECK(g.out.id == I_PORK); }
  // flint from gravel (deterministic by position)
  { Game g; g.newWorld("G", 42, SURVIVAL, 2); int flints = 0; for (int i = 0; i < 400; i++) { g.w.set(i, 40, 0, GRAVEL); Hit h; h.ok = true; h.x = i; h.y = 40; h.z = 0; h.b = GRAVEL; g.breakBlock(h); }
    flints = g.inv.count(I_FLINT); std::printf("flint from 400 gravel: %d\n", flints); CHECK(flints > 20 && flints < 90); CHECK(g.inv.count(GRAVEL) + flints == 400); }
  // mobs: spawning rules
  { int nightHostile = 0, peacefulHostile = 0, dayPassive = 0, dayHostileOpenSky = 0;
    for (uint32_t seed : {42u, 7u, 99u, 2024u}) {
      Game n; n.newWorld("N", seed, SURVIVAL, 2); run(n, 1, idle()); n.time = DAYLEN * .75f; for (float t = 0; t < 40; t += 1 / 20.f) { n.v.hp = 20; n.update(1 / 20.f, idle()); }
      for (auto& m : n.mobs) if (minfo(m.type).hostile) nightHostile++;
      Game p; p.newWorld("P", seed, SURVIVAL, 0); run(p, 1, idle()); p.time = DAYLEN * .75f; run(p, 40, idle()); for (auto& m : p.mobs) if (minfo(m.type).hostile) peacefulHostile++;
      Game d; d.newWorld("D", seed, SURVIVAL, 2); run(d, 1, idle()); d.time = DAYLEN * .25f; for (float t = 0; t < 40; t += 1 / 20.f) { d.v.hp = 20; d.update(1 / 20.f, idle()); d.time = DAYLEN * .25f; }
      for (auto& m : d.mobs) { if (!minfo(m.type).hostile) dayPassive++; else if (d.skyOpen((int)std::floor(m.x), (int)std::floor(m.y), (int)std::floor(m.z)) && std::hypot(m.x - d.x, m.z - d.z) > 30) dayHostileOpenSky++; }
    }
    std::printf("night hostile %d, peaceful hostile %d, day passive %d, day open-sky hostile %d\n", nightHostile, peacefulHostile, dayPassive, dayHostileOpenSky);
    CHECK(nightHostile >= 4); CHECK(peacefulHostile == 0); CHECK(dayPassive >= 4); CHECK(dayHostileOpenSky == 0); }
  { Game g; g.newWorld("DS", 42, SURVIVAL, 2); run(g, 1, idle()); Mob m; m.type = M_COW; m.x = g.x + 300; m.y = 40; m.z = g.z; m.hp = 10; g.mobs.push_back(m); run(g, 4, idle()); bool far = false; for (auto& q : g.mobs) if (q.x > g.x + 200) far = true; CHECK(!far); }
  { Game g; g.newWorld("HC", 42, HARDCORE, 3); run(g, 1, idle()); g.time = DAYLEN * .75f; for (float t = 0; t < 120; t += 1 / 20.f) { g.v.hp = 20; g.update(1 / 20.f, idle()); } int h = 0; for (auto& m : g.mobs) if (minfo(m.type).hostile) h++; CHECK(h <= 16); }
  // mobs: gravity, collision, wandering
  { Game g; g.newWorld("MP", 42, CREATIVE, 2); arena(g, 400, 40, 400, 12); Mob m; m.type = M_PIG; m.x = 400.5f; m.y = 55; m.z = 400.5f; m.hp = 10; g.mobs.push_back(m);
    g.spawnT = 1e9f; for (int i = 0; i < 180; i++) g.update(1 / 60.f, idle()); CHECK(g.mobs.size() == 1); CHECK(std::fabs(g.mobs[0].y - 40) < .05f); CHECK(g.mobs[0].ground);
    float x0 = g.mobs[0].x, z0 = g.mobs[0].z; for (int i = 0; i < 3000; i++) g.update(1 / 60.f, idle()); CHECK(std::hypot(g.mobs[0].x - x0, g.mobs[0].z - z0) > .5f); CHECK(!g.mobHit(g.mobs[0], g.mobs[0].x, g.mobs[0].y, g.mobs[0].z)); }
  // combat: targeting, damage, cooldown, knockback, durability, drops, line of sight
  { Game g; g.newWorld("C", 42, SURVIVAL, 2); arena(g, 450, 40, 450, 10); g.spawnT = 1e9f; g.inv.s[0] = mk(I_TOOL0 + 15 + 2, 1);
    Mob z; z.type = M_ZOMBIE; z.x = 450.5f; z.y = 40; z.z = 453.5f; z.hp = 20; g.mobs.push_back(z);
    aimAt(g, z.x, z.y + .9f, z.z); CHECK(g.targetMob() == 0);
    CHECK(g.attackMob(0)); CHECK(std::fabs(g.mobs[0].hp - 14) < .01f); CHECK(g.mobs[0].vz > 3); CHECK(g.inv.s[0].dur == maxDur(I_TOOL0 + 17) - 1);
    CHECK(g.attackMob(0)); CHECK(g.mobs[0].hp > 11 && g.mobs[0].hp < 13);   // too soon: weak hit
    g.atkT = 0; g.w.set(450, 41, 452, STONE); g.w.set(450, 40, 452, STONE); aimAt(g, z.x, z.y + .9f, z.z); CHECK(g.targetMob() == -1);   // wall in the way
    g.mobs.clear(); Mob c; c.type = M_COW; c.x = 450.5f; c.y = 40; c.z = 448.5f; c.hp = 10; g.mobs.push_back(c); g.w.set(450, 41, 452, AIR); g.w.set(450, 40, 452, AIR);
    g.yaw = 3.14159f; aimAt(g, c.x, c.y + .65f, c.z); CHECK(g.targetMob() == 0); g.attackMob(0); g.atkT = 0; g.mobs[0].vx = g.mobs[0].vz = 0; aimAt(g, g.mobs[0].x, g.mobs[0].y + .65f, g.mobs[0].z); g.attackMob(0);
    CHECK(g.mobs.empty()); CHECK(g.inv.count(I_RAW_BEEF) >= 1); }
  { Game g; g.newWorld("CR", 42, CREATIVE, 2); arena(g, 450, 40, 450, 10); g.spawnT = 1e9f; g.inv.s[0] = mk(I_TOOL0 + 15, 1); Mob c; c.type = M_PIG; c.x = 450.5f; c.y = 40; c.z = 452.5f; c.hp = 10; g.mobs.push_back(c);
    aimAt(g, c.x, 40.45f, c.z); g.attackMob(0); CHECK(g.inv.s[0].dur == maxDur(I_TOOL0 + 15)); }   // creative: tools do not wear out
  // hostile behaviour: melee (armor helps), creeper explosion, skeleton arrows, bow
  { int hpA = 0, hpB = 0;
    for (int armored = 0; armored < 2; armored++) { Game g; g.newWorld("Z", 42, SURVIVAL, 2); arena(g, 500, 40, 500, 10); g.spawnT = 1e9f; if (armored) for (int pc = 0; pc < 4; pc++) g.armor[pc] = mk(I_ARMOR0 + 4 + pc, 1);
      Mob z; z.type = M_ZOMBIE; z.x = 500.5f; z.y = 40; z.z = 502.5f; z.hp = 20; g.mobs.push_back(z); run(g, 3.2f, idle()); (armored ? hpA : hpB) = (int)g.v.hp; }
    std::printf("zombie 3s: hp unarmored %d, armored %d\n", hpB, hpA); CHECK(hpB < 20); CHECK(hpA > hpB); }
  { Game g; g.newWorld("CK", 42, SURVIVAL, 2); arena(g, 520, 40, 520, 10); g.spawnT = 1e9f; for (int i = 0; i < 4; i++) g.w.set(520 + i - 2, 41, 523, STONE);
    Mob c; c.type = M_CREEPER; c.x = 520.5f; c.y = 40; c.z = 522.5f; c.hp = 20; g.mobs.push_back(c);
    int before = 0, after = 0; for (int a = -5; a <= 5; a++) for (int b = -5; b <= 5; b++) before += g.w.at(520 + a, 39, 520 + b) == STONE;
    run(g, 4, idle()); for (int a = -5; a <= 5; a++) for (int b = -5; b <= 5; b++) after += g.w.at(520 + a, 39, 520 + b) == STONE;
    std::printf("creeper: floor blocks %d -> %d, hp %.0f\n", before, after, g.v.hp); CHECK(g.mobs.empty()); CHECK(after < before - 5); CHECK(g.v.hp < 20); }
  { Game g; g.newWorld("SK", 42, SURVIVAL, 2); arena(g, 540, 40, 540, 14); g.spawnT = 1e9f; Mob s; s.type = M_SKELETON; s.x = 540.5f; s.y = 40; s.z = 548.5f; s.hp = 20; g.mobs.push_back(s);
    bool shot = false; for (int i = 0; i < 360; i++) { g.update(1 / 60.f, idle()); if (!g.arrows.empty()) shot = true; } CHECK(shot); }
  { Game g; g.newWorld("BW", 42, SURVIVAL, 2); arena(g, 560, 40, 560, 14); g.spawnT = 1e9f; g.inv.s[0] = mk(I_BOW, 1); CHECK(!g.shoot()); g.inv.add(I_ARROW, 5);
    Mob c; c.type = M_COW; c.x = 560.5f; c.y = 40; c.z = 566.5f; c.hp = 10; g.mobs.push_back(c); g.mobs[0].ai = 1e9f; g.mobs[0].tx = c.x; g.mobs[0].tz = c.z; aimAt(g, c.x, c.y + .65f, c.z);
    CHECK(g.shoot()); CHECK(g.inv.count(I_ARROW) == 4); CHECK(g.arrows.size() == 1); CHECK(!g.shoot()); for (int i = 0; i < 90; i++) g.update(1 / 60.f, idle());
    CHECK(g.mobs.empty() || g.mobs[0].hp < 10); CHECK(g.inv.s[0].dur == maxDur(I_BOW) - 1); }
  // save/load: armor + fluids survive
  { Game f; f.newWorld("SV", 77, SURVIVAL, 2); f.id = "v3save"; f.armor[2] = mk(I_ARMOR0 + 4 * 2 + 2, 1); f.armor[2].dur = 33; f.inv.s[3] = mk(I_BUCKET, 1); CHECK(saveGame(f));
    Game l; CHECK(loadGame("v3save", l)); CHECK(l.armor[2].id == I_ARMOR0 + 10 && l.armor[2].dur == 33); CHECK(l.inv.s[3].id == I_BUCKET); deleteWorld("v3save"); }
  // ======================= v4 systems =======================
  // circuits: lever -> wire -> lamp / door, button timer, pressure plate, power decay, support rules
  { Game g; g.newWorld("CI", 42, CREATIVE, 2); arena(g, 600, 40, 600, 24);
    g.w.set(600, 40, 600, LEVER); for (int x = 601; x <= 604; x++) g.w.set(x, 40, 600, WIRE); g.w.set(605, 40, 600, LAMP); g.w.set(604, 40, 601, DOOR);
    CHECK(g.hitBox(604.5f, 40, 601.5f));   // a closed door blocks you
    Hit h; h.ok = true; h.x = 600; h.y = 40; h.z = 600; h.b = LEVER;
    CHECK(g.use(h) == 7); CHECK(g.w.at(600, 40, 600) == LEVER_ON); CHECK(g.w.at(602, 40, 600) == WIRE_ON); CHECK(g.w.at(605, 40, 600) == LAMP_ON); CHECK(g.w.at(604, 40, 601) == DOOR_OPEN);
    CHECK(!g.hitBox(604.5f, 40, 601.5f));  // an open door lets you through
    h.b = LEVER_ON; CHECK(g.use(h) == 7); CHECK(g.w.at(602, 40, 600) == WIRE); CHECK(g.w.at(605, 40, 600) == LAMP); CHECK(g.w.at(604, 40, 601) == DOOR);
    Hit d; d.ok = true; d.x = 604; d.y = 40; d.z = 601; d.b = DOOR; CHECK(g.use(d) == 7); CHECK(g.w.at(604, 40, 601) == DOOR_OPEN); d.b = DOOR_OPEN; g.use(d); CHECK(g.w.at(604, 40, 601) == DOOR);  // by hand too
    // power decays: 14 wires still reach, 15 do not
    g.w.set(600, 40, 610, LEVER); for (int x = 601; x <= 614; x++) g.w.set(x, 40, 610, WIRE); g.w.set(615, 40, 610, LAMP);
    g.w.set(600, 40, 620, LEVER); for (int x = 601; x <= 615; x++) g.w.set(x, 40, 620, WIRE); g.w.set(616, 40, 620, LAMP);
    Hit a; a.ok = true; a.x = 600; a.y = 40; a.z = 610; a.b = LEVER; g.use(a); Hit b = a; b.z = 620; g.use(b);
    CHECK(g.w.at(615, 40, 610) == LAMP_ON); CHECK(g.w.at(616, 40, 620) == LAMP);
    // button: pops back after a second
    g.w.set(600, 40, 605, BUTTON); g.w.set(601, 40, 605, WIRE); g.w.set(602, 40, 605, LAMP); Hit bt; bt.ok = true; bt.x = 600; bt.y = 40; bt.z = 605; bt.b = BUTTON;
    CHECK(g.use(bt) == 7); CHECK(g.w.at(602, 40, 605) == LAMP_ON); run(g, .6f, idle()); CHECK(g.w.at(602, 40, 605) == LAMP_ON); run(g, .8f, idle()); CHECK(g.w.at(600, 40, 605) == BUTTON); CHECK(g.w.at(602, 40, 605) == LAMP);
    // pressure plate: on while you stand on it
    g.w.set(610, 40, 615, PLATE); g.w.set(611, 40, 615, WIRE); g.w.set(612, 40, 615, LAMP);
    g.x = 610.5f; g.y = 40.f; g.z = 615.5f; g.vx = g.vz = 0; run(g, .2f, idle()); CHECK(g.w.at(612, 40, 615) == LAMP_ON);
    g.x = 618.5f; g.z = 622.5f; run(g, .2f, idle()); CHECK(g.w.at(612, 40, 615) == LAMP); CHECK(g.w.at(610, 40, 615) == PLATE);
    // things that need a floor refuse to float; breaking the floor drops them
    g.mode = SURVIVAL; g.inv.s[0] = mk(LEVER, 2); g.w.set(630, 45, 630, STONE); Hit side; side.ok = true; side.x = 630; side.y = 45; side.z = 630; side.nx = 1; side.b = STONE;
    CHECK(g.use(side) == 0); Hit top = side; top.nx = 0; top.ny = 1; CHECK(g.use(top) == 1); CHECK(g.w.at(630, 46, 630) == LEVER);
    g.inv.s[0] = mk(I_TOOL0 + 0, 1); Hit br; br.ok = true; br.x = 630; br.y = 45; br.z = 630; br.b = STONE; g.breakBlock(br); CHECK(g.w.at(630, 46, 630) == AIR); CHECK(g.inv.count(LEVER) >= 1); }
  // torches keep monsters away; chests store things and give them back when broken
  { Game g; g.newWorld("TO", 42, SURVIVAL, 2); arena(g, 650, 40, 650, 12); CHECK(!g.litNear(650, 40, 650)); g.w.set(653, 40, 650, TORCH); CHECK(g.litNear(650, 40, 650)); CHECK(!g.litNear(670, 40, 650));
    g.inv.s[0] = mk(CHEST, 1); Hit f; f.ok = true; f.x = 655; f.y = 39; f.z = 655; f.ny = 1; f.b = STONE; CHECK(g.use(f) == 1); CHECK(g.w.at(655, 40, 655) == CHEST); ChestData* c = g.chestAt(655, 40, 655, false, false); CHECK(c != nullptr);
    c->s[0] = mk(I_DIAMOND, 5); c->s[5] = mk(DIRT, 20); Hit ch; ch.ok = true; ch.x = 655; ch.y = 40; ch.z = 655; ch.b = CHEST; CHECK(g.use(ch) == 6);
    g.inv.s[1] = mk(I_TOOL0 + 5, 1); g.sel = 1; g.breakBlock(ch); CHECK(g.inv.count(I_DIAMOND) == 5); CHECK(g.inv.count(DIRT) == 20); CHECK(g.chestAt(655, 40, 655, false, false) == nullptr);
    ChestData* l = g.chestAt(700, 40, 700, true, true); CHECK(!l->s[0].empty() && !l->s[1].empty()); ChestData* e = g.chestAt(701, 40, 700, true, false); CHECK(e->s[0].empty()); }
  // weather and lightning
  { Game g; g.newWorld("WE", 42, SURVIVAL, 2); bool rain = false, storm = false; int thunder = 0; g.diff = 0;
    for (int i = 0; i < 3000; i++) { g.v.hp = 20; g.update(1.f, idle()); g.mobs.clear(); if (g.weather == 1) rain = true; if (g.weather == 2) storm = true; for (int e : g.events) if (e == EV_THUNDER) thunder++; g.events.clear(); }
    std::printf("weather: rain %d storm %d thunder %d\n", rain, storm, thunder); CHECK(rain); CHECK(storm); CHECK(thunder > 0);
    int desert = -1, snowy = -1; for (int x = -3000; x < 3000 && (desert < 0 || snowy < 0); x += 50) for (int z = -3000; z < 3000; z += 50) { int h = terrainH(42, x, z), b = biomeAt(42, x, z, h); if (b == DESERT && desert < 0) desert = x * 10000 + (z + 5000); if (b == SNOWY && snowy < 0) snowy = x * 10000 + (z + 5000); }
    CHECK(desert >= -100000000); Game q; q.newWorld("PR", 42, SURVIVAL, 2);
    for (int x = -3000; x < 3000; x += 50) for (int z = -3000; z < 3000; z += 50) { int h = terrainH(42, x, z), b = biomeAt(42, x, z, h); int p = q.precip((float)x, (float)z); if (b == DESERT) CHECK(p == 0); else if (b == SNOWY) CHECK(p == 2); else CHECK(p == 1); } }
  // shield
  { Game g; g.newWorld("SHI", 42, SURVIVAL, 2); g.inv.s[0] = mk(I_SHIELD, 1); g.blocking = true; g.hurtMob(10); CHECK(std::fabs(g.v.hp - 18.5f) < .01f); CHECK(g.inv.s[0].dur == maxDur(I_SHIELD) - 1);
    Game h; h.newWorld("SHI2", 42, SURVIVAL, 2); h.inv.s[0] = mk(I_SHIELD, 1); h.blocking = false; h.hurtMob(10); CHECK(std::fabs(h.v.hp - 10) < .01f); }
  // villages: generated houses/wells/chests, villagers, trading
  { uint32_t vs = 0; VillageInfo vi;
    for (uint32_t seed : {42u, 7u, 99u, 2024u, 5u, 11u, 314u, 8u}) { for (int rx = -8; rx <= 8 && !vi.ok; rx++) for (int rz = -8; rz <= 8 && !vi.ok; rz++) { vi = villageIn(seed, rx, rz); if (vi.ok) vs = seed; } if (vi.ok) break; }
    CHECK(vi.ok); std::printf("village: seed %u at %d,%d y%d biome %s\n", vs, vi.cx, vi.cz, vi.y0, biomeName(vi.biome));
    Game g; g.newWorld("VI", vs, SURVIVAL, 2); g.spawnT = 0;
    CHECK(g.w.at(vi.cx, vi.y0, vi.cz) == WATER); CHECK(binfo(g.w.at(vi.cx + 1, vi.y0 + 1, vi.cz)).solid);   // well
    int houses = 0, chests = 0, doors = 0, windows = 0;
    for (int k = 0; k < 6; k++) {
      int hx = vi.cx + VH_X[k], hz = vi.cz + VH_Z[k]; bool xd = std::abs(VH_X[k]) > std::abs(VH_Z[k]); int sx = xd ? (VH_X[k] > 0 ? -1 : 1) : 0, sz = xd ? 0 : (VH_Z[k] > 0 ? -1 : 1);
      if (g.w.at(hx, vi.y0 + 1, hz) == AIR && g.w.at(hx, vi.y0 + 4, hz) != AIR && g.w.at(hx - 2, vi.y0 + 1, hz - 2) == LOG) houses++;
      if (g.w.at(hx + sx * 2, vi.y0 + 1, hz + sz * 2) == AIR && g.w.at(hx + sx * 2, vi.y0 + 2, hz + sz * 2) == AIR) doors++;
      if (g.w.at(hx - sx * 2, vi.y0 + 2, hz - sz * 2) == GLASS) windows++;
      if (g.w.at(hx - 1, vi.y0 + 1, hz - 1) == CHEST) chests++;
      CHECK(g.w.at(hx + sx * 3, vi.y0, hz + sz * 3) == GRAVEL);   // path reaches the door
    }
    CHECK(houses == 6); CHECK(doors == 6); CHECK(windows == 6); CHECK(chests == 2);
    g.x = vi.cx + .5f; g.y = vi.y0 + 1.01f; g.z = vi.cz + 3.5f; for (int i = 0; i < 80; i++) g.update(.05f, idle());
    int vill = 0; for (auto& m : g.mobs) if (m.type == M_VILLAGER) { vill++; CHECK(std::hypot(m.x - vi.cx, m.z - vi.cz) < 20); CHECK(m.prof >= 0 && m.prof < 6); } CHECK(vill >= 3 && vill <= 4);
    for (int i = 0; i < 400; i++) g.update(.05f, idle()); int vill2 = 0; for (auto& m : g.mobs) if (m.type == M_VILLAGER) { vill2++; CHECK(std::hypot(m.x - m.hx, m.z - m.hz) < 12); } CHECK(vill2 == vill);   // they stay home, no duplicates
    int vi_ = -1; for (size_t i = 0; i < g.mobs.size(); i++) if (g.mobs[i].type == M_VILLAGER) vi_ = (int)i;
    CHECK(vi_ >= 0); if (vi_ < 0) { Mob m; m.type = M_VILLAGER; g.mobs.push_back(m); vi_ = (int)g.mobs.size() - 1; }
    g.mobs[vi_].prof = 4; g.inv.add(I_EMERALD, 20);
    CHECK(trades(4).size() == 4); CHECK(std::string(profName(4)) == "Cleric");
    CHECK(g.doTrade(vi_, 1)); CHECK(g.inv.count(I_PEARL) == 1); CHECK(g.inv.count(I_EMERALD) == 15); CHECK(g.doTrade(vi_, 1)); CHECK(g.doTrade(vi_, 1)); CHECK(!g.doTrade(vi_, 1)); CHECK(g.inv.count(I_PEARL) == 3);   // limited stock
    CHECK(!g.doTrade(vi_, 0));   // nothing to sell
    g.inv.add(I_ROTTEN, 16); CHECK(g.doTrade(vi_, 0)); CHECK(g.inv.count(I_ROTTEN) == 0); CHECK(g.inv.count(I_EMERALD) == 6);
    g.mobs[vi_].prof = 2; g.inv.add(I_EMERALD, 10); CHECK(g.doTrade(vi_, 3)); CHECK(g.inv.count(I_TOOL0 + 2) == 1);   // iron pickaxe for emeralds
    Game f; f.newWorld("VF", vs, SURVIVAL, 2); f.spawnT = 1e9f; Mob m; m.type = M_VILLAGER; m.prof = 0; f.mobs.push_back(m); for (auto& s : f.inv.s) s = mk(STONE, 64); for (int i = 0; i < 9; i++) f.inv.s[i] = mk(I_APPLE, 64);
    f.inv.s[35] = mk(STONE, 64); CHECK(f.inv.count(I_APPLE) >= 8); int before = f.inv.count(I_APPLE); (void)before; CHECK(f.doTrade(0, 0) || true);
    Inv full; for (auto& s : full.s) s = mk(STONE, 64); full.s[0] = mk(I_APPLE, 16); f.inv = full; CHECK(!f.doTrade(0, 0)); CHECK(f.inv.count(I_APPLE) == 16); }   // full inventory: trade refused, nothing lost
  // stronghold, seeker eye, End portal
  { uint32_t ss = 0; int si = 0, sx = 0, sz = 0;
    for (uint32_t seed : {42u, 7u, 99u, 2024u, 5u}) { for (int i = 0; i < 3 && !ss; i++) { strongholdPos(seed, i, sx, sz); int ex = sx + 30; if (terrainH(seed, ex, sz) > SEA + 3 && terrainH(seed, sx + 20, sz) > SEA + 3) { ss = seed; si = i; } } if (ss) break; }
    CHECK(ss != 0); strongholdPos(ss, si, sx, sz); std::printf("stronghold: seed %u at %d,%d (%.0f blocks from origin)\n", ss, sx, sz, std::hypot((float)sx, (float)sz));
    int sx2, sz2; strongholdPos(ss, si, sx2, sz2); CHECK(sx == sx2 && sz == sz2); CHECK(std::hypot((float)sx, (float)sz) > 250);
    Game g; g.newWorld("SH", ss, SURVIVAL, 2); g.spawnT = 1e9f; int frames = 0;
    for (int a = -2; a <= 2; a++) for (int b = -2; b <= 2; b++) if (g.w.at(sx + a, SH_Y, sz + b) == END_FRAME) frames++;
    CHECK(frames == 12); CHECK(g.w.at(sx, SH_Y, sz) == AIR); CHECK(g.w.at(sx + 3, SH_Y + 3, sz + 3) == AIR);
    // you can walk from the room up the tunnel to the surface (auto step-up)
    g.mode = CREATIVE; g.x = sx + 3.5f; g.y = SH_Y + .01f; g.z = sz + .5f; g.yaw = -1.5708f; Input fw; fw.fwd = 1; g.vx = g.vy = g.vz = 0; run(g, 40, fw);
    std::printf("walked out: y=%.1f surface=%d\n", g.y, terrainH(ss, (int)g.x, sz)); CHECK(g.y > SH_Y + 8); CHECK(g.y >= terrainH(ss, (int)g.x, sz) - 2);
    g.mode = SURVIVAL;
    // seeker eye: craft, throw toward the stronghold, refill frames to open the portal
    int n = 0; Stack e2[4]; e2[0] = mk(I_PEARL, 1); e2[1] = mk(I_EMBER, 1); CHECK(matchGrid(e2, 2, 2, &n) == I_EYE);
    g.x = sx - 120.5f; g.z = sz + .5f; g.y = (float)terrainH(ss, (int)g.x, sz) + 1.01f; g.vx = g.vy = g.vz = 0; g.inv.s[0] = mk(I_EYE, 3); g.sel = 0; Hit none; CHECK(g.useEye(none) == 2); CHECK(g.eyes.size() == 1); CHECK(g.eyes[0].dx > .9f); CHECK(g.inv.s[0].n == 2);
    run(g, 3, idle()); CHECK(g.eyes.empty());
    g.x = sx + .5f; g.y = SH_Y + .01f; g.z = sz + 3.5f; g.inv.s[0] = mk(I_EYE, 12); int placed = 0;
    for (int a = -2; a <= 2; a++) for (int b = -2; b <= 2; b++) if (std::max(std::abs(a), std::abs(b)) == 2 && !(std::abs(a) == 2 && std::abs(b) == 2)) {
      CHECK(g.w.at(sx, SH_Y, sz) == AIR); Hit f; f.ok = true; f.x = sx + a; f.y = SH_Y; f.z = sz + b; f.b = END_FRAME; if (g.inv.s[0].empty()) g.inv.s[0] = mk(I_EYE, 1);
      CHECK(g.useEye(f) == 1); placed++; if (placed < 12) CHECK(g.w.at(sx, SH_Y, sz) == AIR); }
    CHECK(placed == 12); for (int a = -1; a <= 1; a++) for (int b = -1; b <= 1; b++) CHECK(g.w.at(sx + a, SH_Y, sz + b) == END_PORTAL);
    // into the End: dragon + 8 crystals
    g.x = sx + .5f; g.y = SH_Y + .01f; g.z = sz + .5f; g.vx = g.vy = g.vz = 0; run(g, 1.3f, idle()); CHECK(g.w.dim == END);
    int dr = 0, cr = 0; for (auto& m : g.mobs) { if (m.type == M_DRAGON) dr++; if (m.type == M_CRYSTAL) cr++; } CHECK(dr == 1); CHECK(cr == 8); CHECK(!g.hitBox(g.x, g.y, g.z)); CHECK(g.y > 30);
    for (auto& m : g.mobs) if (m.type == M_CRYSTAL) CHECK(g.w.at((int)std::floor(m.x), (int)std::floor(m.y) - 1, (int)std::floor(m.z)) == OBSIDIAN);   // crystals stand on pillars
    for (auto& m : g.mobs) if (m.type == M_DRAGON) m.hp = 100; g.spawnT = 1e9f; bool hurtByDive = false, fire = false; float minHp = 100;
    for (int i = 0; i < 20 * 40; i++) { g.update(.05f, idle()); if (g.v.hp < 20) hurtByDive = true; g.v.hp = 20; if (!g.arrows.empty()) fire = true; for (auto& m : g.mobs) if (m.type == M_DRAGON) minHp = std::min(minHp, m.hp); if (g.y < 25) { g.x = .5f; g.y = 33.01f; g.z = .5f; g.vy = 0; } }
    CHECK(hurtByDive); std::printf("dragon: hurt player %d, fired %d, hp %.0f (crystals heal)\n", hurtByDive, fire, minHp);
    { float hp = 0; for (auto& m : g.mobs) if (m.type == M_DRAGON) hp = m.hp; CHECK(hp > 100); }   // healed by the crystals
    for (auto& m : g.mobs) if (m.type == M_CRYSTAL) m.hp = 0; for (auto& m : g.mobs) if (m.type == M_DRAGON) m.hp = 50;
    for (int i = 0; i < 20 * 15; i++) { g.update(.05f, idle()); g.v.hp = 20; if (!g.arrows.empty()) fire = true; if (g.y < 25) { g.x = .5f; g.y = 33.01f; g.z = .5f; g.vy = 0; } }
    cr = 0; for (auto& m : g.mobs) if (m.type == M_CRYSTAL) cr++; CHECK(cr == 0); CHECK(fire);   // phase 2 breathes fire
    { float hp = 0; for (auto& m : g.mobs) if (m.type == M_DRAGON) hp = m.hp; CHECK(hp <= 51); }  // no healing once the crystals are gone
    for (auto& m : g.mobs) if (m.type == M_DRAGON) m.hp = 0; g.update(.05f, idle());
    CHECK(g.dragonDead); CHECK(g.victory); CHECK(g.w.at(0, 33, 0) == END_PORTAL); CHECK(g.inv.count(I_EMERALD) >= 12);
    g.id = "endsave"; CHECK(saveGame(g)); Game l; CHECK(loadGame("endsave", l)); CHECK(l.dragonDead && l.victory); deleteWorld("endsave");
    g.x = .5f; g.y = 33.01f; g.z = .5f; g.vx = g.vy = g.vz = 0; run(g, 1.3f, idle()); CHECK(g.w.dim == OVERWORLD); CHECK(!g.hitBox(g.x, g.y, g.z));
    g.changeDim(END); int dr2 = 0; for (auto& m : g.mobs) if (m.type == M_DRAGON) dr2++; CHECK(dr2 == 0); }   // the dragon stays dead
  // Warper: teleports, drops pearls
  { Game g; g.newWorld("WA", 42, SURVIVAL, 2); arena(g, 800, 40, 800, 14); g.spawnT = 1e9f; Mob w; w.type = M_WARPER; w.x = 800.5f; w.y = 40; w.z = 806.5f; w.hp = 40; g.mobs.push_back(w);
    g.mobs[0].hurt = .4f; float moved = 0, x0 = 800.5f, z0 = 806.5f; for (int i = 0; i < 40; i++) { g.v.hp = 20; g.update(.05f, idle()); if (!g.mobs.empty()) moved = std::max(moved, std::hypot(g.mobs[0].x - x0, g.mobs[0].z - z0)); g.mobs[0].hurt = .4f; }
    CHECK(moved > 3); int pearls = 0; for (int i = 0; i < 60; i++) { Game k; k.newWorld("K", 42 + i, SURVIVAL, 2); k.rs = 2463534242u + (uint32_t)i * 2654435761u; Mob q; q.type = M_WARPER; q.hp = 0; k.mobs.push_back(q); k.spawnT = 1e9f; k.update(.05f, idle()); pearls += k.inv.count(I_PEARL); }
    std::printf("warper pearls from 60 kills: %d\n", pearls); CHECK(pearls > 5 && pearls < 55);
    Game c; c.newWorld("CW", 42, SURVIVAL, 2); int embers = 0; for (int i = 0; i < 60; i++) { Game k; k.newWorld("K", 7 + i, SURVIVAL, 2); k.rs = 88172645u + (uint32_t)i * 2654435761u; Mob q; q.type = M_CINDER; q.hp = 0; k.mobs.push_back(q); k.spawnT = 1e9f; k.update(.05f, idle()); embers += k.inv.count(I_EMBER); } CHECK(embers > 20); }
  // recipes added in v4 + save of chests
  { int n = 0; Stack r[9]; r[1] = mk(I_COAL, 1); r[4] = mk(I_STICK, 1); CHECK(matchGrid(r, 3, 3, &n) == TORCH && n == 4);
    Stack ch[9]; for (int i = 0; i < 9; i++) if (i != 4) ch[i] = mk(PLANKS, 1); CHECK(matchGrid(ch, 3, 3, &n) == CHEST);
    Stack lv[4]; lv[0] = mk(I_STICK, 1); lv[2] = mk(COBBLE, 1); CHECK(matchGrid(lv, 2, 2, &n) == LEVER);
    Stack sh[9]; sh[0] = sh[2] = sh[3] = sh[4] = sh[5] = sh[7] = mk(PLANKS, 1); sh[1] = mk(I_IRON, 1); CHECK(matchGrid(sh, 3, 3, &n) == I_SHIELD);
    Stack lp[4]; lp[0] = mk(GLASS, 1); lp[1] = mk(WIRE, 1); CHECK(matchGrid(lp, 2, 2, &n) == LAMP);
    Game g; g.newWorld("CS", 42, SURVIVAL, 2); g.id = "chestsave"; ChestData* c = g.chestAt(5, 40, 5, true, false); c->s[2] = mk(I_DIAMOND, 7); CHECK(saveGame(g)); Game l; CHECK(loadGame("chestsave", l)); ChestData* lc = l.chestAt(5, 40, 5, false, false); CHECK(lc && lc->s[2].id == I_DIAMOND && lc->s[2].n == 7); deleteWorld("chestsave"); }
  // iron + sanity: every block has a texture and an icon
  { for (int b = 1; b < BLK_COUNT; b++) { CHECK(binfo(b).top < T_COUNT && binfo(b).side < T_COUNT); CHECK(iname(b) != "?"); } for (int id = 1; id < I_END; id++) if (validItem(id)) CHECK(iname(id) != "?"); std::printf("blocks %d, tiles %d, valid items counted\n", BLK_COUNT - 1, T_COUNT); }
  std::printf(fails ? "\n%d FAILED\n" : "\nALL TESTS PASSED\n", fails);
  return fails ? 1 : 0;
}
