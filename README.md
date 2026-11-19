# Voxel3D — C++ / raylib survival sandbox

A 3D voxel survival game in C++17 with raylib. Original code and original procedurally drawn pixel art
(no Minecraft/Mojang assets of any kind).

## Status: version 4

**Working (game logic covered by automated tests in `tests/test_logic.cpp`):**
- **Modes & worlds:** Survival (empty start), Creative, Hardcore (permanent lock on death); many independent worlds, autosave, full save of blocks, inventory, armor, chests, furnaces, fluids, death piles, End progress.
- **Survival loop:** wood -> tools -> stone -> ores -> furnace -> iron/diamond gear; hunger, health, drowning, fall/lava damage, food, 5 tool tiers with durability.
- **Fluids:** water and lava flow (falling, spreading, drying up), buckets, water+lava = obsidian/cobblestone.
- **Mobs:** cow, pig, sheep, chicken; Shambler, Bone Archer (arrows), Crawler, Fuse Beast (explodes), Cinder Walker (Nether), **Warper** (teleporting), **villagers**. Night/dark/Nether spawning, torches keep monsters away.
- **Combat:** swords/axes, cooldown, crits, knockback, bow & arrows, **shield blocking**, armor (4 tiers x 4 pieces).
- **Villages:** generated from the seed (well, 6 houses with doors/windows/torches, gravel paths, storage chests). **Villagers** (Farmer, Butcher, Toolsmith, Armorer, Cleric, Fletcher) with a trading screen, emerald currency, limited stock.
- **Nether portal:** obsidian frame + flint & steel -> Nether and back (8:1 scale, auto-built return portal).
- **End progression:** Warper drops Warp Pearl, Cinder Walker drops Ember Rod -> **Seeker Eye** (also bought from Clerics). Use it in the air to point toward the nearest of 3 underground **strongholds** (reached by a long stair tunnel); fill its 12 portal frames with eyes to open the **End portal**.
- **The End & boss:** floating island, 8 obsidian pillars with healing **Sky Crystals**, the **Sky Wyrm** dragon (circling, diving attacks, fire breath at half health, health bar), exit portal, rewards, victory message; the dragon stays dead.
- **Circuits (redstone-style):** lever, button (1 s pulse), pressure plate, circuit wire (power decays over 14 blocks), lamp, door; only the connected circuit is recalculated.
- **Chests** (27 slots, village chests hold loot), **torches**, **weather** (rain, snow in snowy biomes, none in deserts, thunderstorms with lightning strikes), **sound effects** (generated in code) with a volume setting.
- **UI:** inventory with drag & drop / split / armor slots / tooltips / durability bars, 2x2 + 3x3 crafting + recipe book, furnace UI, chest UI, trade UI, ~120 original pixel-art icons, textured blocks, 4 render distances, Vibrant Visuals toggle, first/third person, 6 skins, touch controls.

**Not built (be aware):** pistons/comparators/repeaters, anvil/brewing/enchanting/other workstations, crop farming, music, a real light-propagation system (torches brighten nearby dark faces in the same chunk; at night everything is still dimmed by the day/night tint), mob persistence in saves (mobs respawn), custom skin import, controls remapping, End cities / Nether fortresses / dungeons / mineshafts.

## Controls
WASD move · mouse look · Space jump (double-tap = fly in Creative) · Shift sprint · C/Ctrl sneak / fly down · LMB mine · RMB place/use/eat ·
1-9 / wheel hotbar · E inventory · F5 camera · F3 debug · Esc pause · P dimension (Creative only).

## Build
Desktop (Linux/macOS/Windows with CMake + a C++17 compiler; raylib is downloaded automatically):
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
./build/Voxel3D
```
Logic tests (no graphics, no downloads):
```bash
g++ -std=c++17 -I. tests/test_logic.cpp -o logic_tests && ./logic_tests
```
**Launching from GitHub: see `HOW_TO_LAUNCH.md`. Android APK step by step: `HOW_TO_BUILD_APK.md`.**

## Layout
- `game.h` — all rules: blocks, items, terrain, world, physics, survival, crafting, furnace, saves.
- `art.h` — procedural pixel-art textures and icons.
- `main.cpp` — raylib frontend (rendering, input, menus, HUD, inventory UI, touch controls).
- `tests/` — logic tests + a tiny raylib API stub used only for syntax checks.
- `legacy/main_original.cpp` — the original prototype, kept for reference.

## Honest testing notes
Game logic is unit-tested (fluids, mobs, combat, armor, portals, saving, crafting...). The raylib frontend (`main.cpp`) was syntax-checked against the raylib API but **has not been run on a real GPU/device yet**.
Expect to fix small visual/input issues on first run; send the compiler output or a description and they will be fixed.
