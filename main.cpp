// main.cpp - Voxel3D raylib frontend. All game rules live in game.h; pixel art in art.h.
#include <ctime>
#include <set>

#include "art.h"
#include "game.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
using namespace vx;

enum Screen { S_TITLE, S_CREATE, S_WORLDS, S_SETTINGS, S_SKINS, S_CONTROLS, S_PLAY, S_PAUSE, S_INV, S_DEAD, S_TRADE };
static Screen scr = S_TITLE, prevScr = S_TITLE;
static Settings st;
static Game g;
static Texture2D atlasN, atlasV, iconTex;
static Material mat;
static Camera3D cam;
static bool mobile = false, quitting = false, cursorOff = false, debug = false;
static int viewMode = 0;  // 0 first person, 1 third behind, 2 third front
static std::string toast;
static float toastT = 0, saveTimer = 0, bobT = 0;

struct Skin { const char* name; Color shirt, pants, skin, hair; };
static const Skin SKINS[6] = {
    {"Explorer", {58, 110, 165, 255}, {50, 60, 110, 255}, {224, 180, 138, 255}, {80, 50, 30, 255}},
    {"Ranger", {70, 140, 80, 255}, {90, 70, 50, 255}, {210, 165, 120, 255}, {60, 40, 25, 255}},
    {"Knight", {150, 150, 160, 255}, {90, 90, 100, 255}, {224, 180, 138, 255}, {40, 40, 40, 255}},
    {"Mage", {110, 60, 150, 255}, {50, 40, 80, 255}, {200, 160, 130, 255}, {230, 230, 240, 255}},
    {"Robot", {170, 175, 185, 255}, {110, 115, 125, 255}, {140, 200, 215, 255}, {200, 60, 60, 255}},
    {"Fox", {210, 110, 40, 255}, {130, 70, 30, 255}, {240, 200, 150, 255}, {225, 100, 30, 255}}};

static void say(const std::string& s, float t = 2.f) { toast = s; toastT = t; }

// ---------------------------------------------------------------- audio (all sounds are generated in code)
static Sound sfx[EV_COUNT];
static bool audioOK = false;
struct SfxDef { float f0, f1, noise, dur, vol; };
static const SfxDef SFX[EV_COUNT] = {
    {160, 70, .6f, .18f, .9f},   // break
    {220, 160, .2f, .08f, .7f},  // place
    {600, 1100, 0, .12f, .5f},   // pickup
    {300, 100, .3f, .25f, .9f},  // hurt
    {900, 300, .5f, .10f, .6f},  // attack
    {200, 120, .4f, .15f, .7f},  // mob hurt
    {90, 30, .85f, .7f, 1.f},    // explosion
    {200, 500, .1f, .9f, .6f},   // portal
    {140, 100, .3f, .2f, .6f},   // door
    {1000, 800, 0, .04f, .5f},   // click
    {500, 200, .3f, .2f, .6f},   // shoot
    {70, 25, .9f, 1.2f, 1.f},    // thunder
    {250, 350, .5f, .08f, .6f},  // eat
    {110, 60, .3f, 1.0f, 1.f},   // dragon
    {500, 900, 0, .15f, .5f}};   // craft / trade
static void buildAudio() {
  InitAudioDevice();
  audioOK = IsAudioDeviceReady();
  if (!audioOK) return;
  for (int e = 0; e < EV_COUNT; e++) {
    const SfxDef& d = SFX[e];
    const unsigned rate = 22050;
    unsigned n = (unsigned)(d.dur * rate);
    std::vector<short> pcm(n);
    float ph = 0;
    uint32_t r = 12345u + e * 7919u;
    for (unsigned i = 0; i < n; i++) {
      float t = (float)i / rate, k = t / d.dur, f = d.f0 + (d.f1 - d.f0) * k;
      ph += 6.2831853f * f / rate;
      r = r * 1664525u + 1013904223u;
      float nz = ((r >> 8) & 0xFFFF) / 32768.f - 1.f, env = (1 - k) * (1 - k) * std::min(1.f, t * 200.f);
      pcm[i] = (short)(((std::sin(ph) * (1 - d.noise) + nz * d.noise) * env * .6f) * 32767);
    }
    Wave w{n, rate, 16, 1, pcm.data()};
    sfx[e] = LoadSoundFromWave(w);  // copies the samples
    SetSoundVolume(sfx[e], d.vol);
  }
}
static void playSfx(int e) { if (audioOK && st.vol > 0 && e >= 0 && e < EV_COUNT) PlaySound(sfx[e]); }

// ---------------------------------------------------------------- settings
static void saveSettings() {
  std::error_code ec;
  std::filesystem::create_directories(saveDir(), ec);
  std::ofstream f(saveDir() + "/settings.txt");
  f << st.ser();
}
static void loadSettings() {
  std::ifstream f(saveDir() + "/settings.txt");
  std::stringstream ss;
  ss << f.rdbuf();
  st.parse(ss.str());
}

// ---------------------------------------------------------------- textures
static Texture2D uploadPixels(const std::vector<Px>& buf, int w, int h) {
  Image img{(void*)buf.data(), w, h, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
  Texture2D t = LoadTextureFromImage(img);  // copies to the GPU; our vector keeps ownership of the pixels
  SetTextureFilter(t, TEXTURE_FILTER_POINT);
  return t;
}
static void buildTextures() {
  const int W = 128;
  std::vector<Px> a(W * W, 0), v(W * W, 0);
  for (int t = 0; t < T_COUNT; t++) {
    Tex tx = makeTile(t), tv = saturated(tx, 1.5f);
    for (int y = 0; y < 16; y++)
      for (int x = 0; x < 16; x++) {
        a[(t / 8 * 16 + y) * W + (t % 8) * 16 + x] = tx.p[y * 16 + x];
        v[(t / 8 * 16 + y) * W + (t % 8) * 16 + x] = tv.p[y * 16 + x];
      }
  }
  atlasN = uploadPixels(a, W, W);
  atlasV = uploadPixels(v, W, W);
  std::vector<Px> ic(256 * 256, 0);
  for (int id = 1; id < I_END; id++) {
    if (!validItem(id)) continue;
    Tex tx = makeIcon(id);
    for (int y = 0; y < 16; y++)
      for (int x = 0; x < 16; x++) ic[((id / 16) * 16 + y) * 256 + (id % 16) * 16 + x] = tx.p[y * 16 + x];
  }
  iconTex = uploadPixels(ic, 256, 256);
  mat = LoadMaterialDefault();
}

// ---------------------------------------------------------------- chunk meshing
struct MB { std::vector<float> v, uv; std::vector<unsigned char> c; std::vector<unsigned short> i; };
struct CM { uint32_t ver = 0; std::vector<Mesh> op, tr; };
static std::unordered_map<uint64_t, CM> meshes;

static const int FD[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
static const int FV[6][12] = {{1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1}, {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0},
                              {0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1}, {0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1}, {1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0}};
static const float FS[6] = {.8f, .8f, 1.f, .5f, .65f, .65f};
static void faceUV(int f, int cx, int cy, int cz, float& u, float& v) {
  switch (f) {
    case 0: u = (float)(1 - cz); v = (float)(1 - cy); break;
    case 1: u = (float)cz; v = (float)(1 - cy); break;
    case 2: u = (float)cx; v = (float)cz; break;
    case 3: u = (float)cx; v = (float)(1 - cz); break;
    case 4: u = (float)cx; v = (float)(1 - cy); break;
    default: u = (float)(1 - cx); v = (float)(1 - cy); break;
  }
}
static void pushFace(MB& m, int x, int y, int z, int f, int tile, float r, float g_, float b, float a, float bw = 1.f, float bh = 1.f) {
  float u0 = (tile % 8) / 8.f, v0 = (tile / 8) / 8.f, e = .002f, sz = .125f - 2 * e;
  unsigned short base = (unsigned short)(m.v.size() / 3);
  for (int k = 0; k < 4; k++) {
    int cx = FV[f][k * 3], cy = FV[f][k * 3 + 1], cz = FV[f][k * 3 + 2];
    m.v.push_back(x + .5f + (cx - .5f) * bw); m.v.push_back(y + cy * bh); m.v.push_back(z + .5f + (cz - .5f) * bw);
    float u, v;
    faceUV(f, cx, cy, cz, u, v);
    if (bh < 1.f && f != 2 && f != 3) v = 1.f - cy * bh;
    m.uv.push_back(u0 + e + u * sz); m.uv.push_back(v0 + e + v * sz);
    m.c.push_back((unsigned char)(std::min(1.f, r) * 255)); m.c.push_back((unsigned char)(std::min(1.f, g_) * 255));
    m.c.push_back((unsigned char)(std::min(1.f, b) * 255)); m.c.push_back((unsigned char)(a * 255));
  }
  const unsigned short ix[6] = {0, 1, 2, 0, 2, 3};
  for (unsigned short q : ix) m.i.push_back((unsigned short)(base + q));
}
static Mesh uploadMB(MB& m) {
  Mesh me{};
  me.vertexCount = (int)m.v.size() / 3;
  me.triangleCount = (int)m.i.size() / 3;
  me.vertices = (float*)MemAlloc((unsigned int)(m.v.size() * sizeof(float)));
  me.texcoords = (float*)MemAlloc((unsigned int)(m.uv.size() * sizeof(float)));
  me.colors = (unsigned char*)MemAlloc((unsigned int)m.c.size());
  me.indices = (unsigned short*)MemAlloc((unsigned int)(m.i.size() * sizeof(unsigned short)));
  std::memcpy(me.vertices, m.v.data(), m.v.size() * sizeof(float));
  std::memcpy(me.texcoords, m.uv.data(), m.uv.size() * sizeof(float));
  std::memcpy(me.colors, m.c.data(), m.c.size());
  std::memcpy(me.indices, m.i.data(), m.i.size() * sizeof(unsigned short));
  UploadMesh(&me, false);
  return me;
}
static void freeCM(CM& cm) {
  for (auto& m : cm.op) UnloadMesh(m);
  for (auto& m : cm.tr) UnloadMesh(m);
  cm.op.clear();
  cm.tr.clear();
}
static void clearMeshes() {
  for (auto& kv : meshes) freeCM(kv.second);
  meshes.clear();
}
static void buildChunk(int cx, int cz) {
  Chunk& c = g.w.chunk(cx, cz);
  uint8_t top[CS * CS];
  for (int x = 0; x < CS; x++)
    for (int z = 0; z < CS; z++) {
      int t = 0;
      for (int y = WH - 1; y > 0; y--) if (binfo(c.d[idx(x, y, z)]).solid) { t = y; break; }
      top[z * CS + x] = (uint8_t)t;
    }
  int bi = PLAINS;
  if (g.w.dim == OVERWORLD) bi = biomeAt(g.w.seed, cx * CS + 8, cz * CS + 8, terrainH(g.w.seed, cx * CS + 8, cz * CS + 8));
  float fr = 1, fg = 1, fb = 1;
  switch (bi) {
    case DESERT: fr = .95f; fg = .85f; fb = .55f; break;
    case SNOWY: fr = .62f; fg = .85f; fb = .8f; break;
    case FOREST: fr = .75f; fg = .92f; fb = .65f; break;
    case MOUNTAINS: fr = .82f; fg = .92f; fb = .82f; break;
    default: break;
  }
  if (!st.vibrant) { fr *= .88f; fg *= .93f; fb *= .85f; }
  std::vector<MB> O(1), T(1);
  const int ox = cx * CS, oz = cz * CS;
  std::vector<std::array<float, 3>> emit;  // torches / lit lamps / glowstone light up nearby dark faces
  for (int y = 0; y < WH; y++) for (int z = 0; z < CS; z++) for (int x = 0; x < CS; x++) {
    uint8_t e = c.d[idx(x, y, z)];
    if (e == TORCH || e == LAMP_ON || e == GLOWSTONE) emit.push_back({ox + x + .5f, y + .5f, oz + z + .5f});
  }
  for (int y = 0; y < WH; y++)
    for (int z = 0; z < CS; z++)
      for (int x = 0; x < CS; x++) {
        uint8_t b = c.d[idx(x, y, z)];
        if (!b) continue;
        const BInfo& B = binfo(b);
        std::vector<MB>& parts = B.trans ? T : O;
        float bw, bh;
        blockShape(b, bw, bh);
        const bool part = bw < 1 || bh < 1;
        for (int f = 0; f < 6; f++) {
          int nx = x + FD[f][0], ny = y + FD[f][1], nz = z + FD[f][2];
          bool inside = nx >= 0 && nx < CS && nz >= 0 && nz < CS && ny >= 0 && ny < WH;
          uint8_t nb = inside ? c.d[idx(nx, ny, nz)] : g.w.at(ox + nx, ny, oz + nz);
          if (!part && nb && !(binfo(nb).trans && nb != b)) continue;
          int tile = f == 2 ? B.top : f == 3 ? B.bot : B.side;
          float lit = 1.f;
          if (!glows(b) && inside && ny <= top[nz * CS + nx] && g.w.dim == OVERWORLD) lit = .35f;  // covered cells are dark (caves)
          if (!emit.empty() && lit < 1.f) {
            float bo = 0, fx_ = ox + x + .5f + FD[f][0] * .5f, fy_ = y + .5f + FD[f][1] * .5f, fz_ = oz + z + .5f + FD[f][2] * .5f;
            for (auto& e : emit) { float d = std::sqrt((fx_ - e[0]) * (fx_ - e[0]) + (fy_ - e[1]) * (fy_ - e[1]) + (fz_ - e[2]) * (fz_ - e[2])); bo = std::max(bo, 1 - d / 7); }
            lit = std::max(lit, bo);
          }
          float s = FS[f] * lit, r = s, gg = s, bb = s, a = 1.f;
          if ((b == GRASS && f == 2) || b == LEAVES) { r *= fr; gg *= fg; bb *= fb; }
          if (b == WATER) { a = .75f; if (st.vibrant) r *= .8f; }
          if (glows(b)) r = gg = bb = 1.f;
          if (parts.back().v.size() / 3 + 4 > 65000) parts.emplace_back();
          pushFace(parts.back(), ox + x, y, oz + z, f, tile, r, gg, bb, a, bw, bh);
        }
      }
  CM& cm = meshes[ckey(cx, cz)];
  freeCM(cm);
  for (auto& m : O) if (!m.v.empty()) cm.op.push_back(uploadMB(m));
  for (auto& m : T) if (!m.v.empty()) cm.tr.push_back(uploadMB(m));
  cm.ver = c.ver;
}
static void rebuildNear(int bx, int bz) {
  int cx = bx >> 4, cz = bz >> 4;
  for (int dx = -1; dx <= 1; dx++)
    for (int dz = -1; dz <= 1; dz++) {
      auto it = meshes.find(ckey(cx + dx, cz + dz));
      if (it != meshes.end() && g.w.loaded(cx + dx, cz + dz) && it->second.ver != g.w.chunk(cx + dx, cz + dz).ver) buildChunk(cx + dx, cz + dz);
    }
}

// ---------------------------------------------------------------- camera / sky / world drawing
static Vector3 fwdVec() {
  float cp = cosf(g.pitch);
  return {-sinf(g.yaw) * cp, sinf(g.pitch), cosf(g.yaw) * cp};
}
static void updateCamera(float dt) {
  Vector3 f = fwdVec();
  float bob = 0;
  if (st.bob && viewMode == 0 && g.ground && scr == S_PLAY) {
    float sp = std::hypot(g.vx, g.vz);
    bobT += sp * dt * 2.2f;
    bob = sinf(bobT) * .03f * std::min(1.f, sp / 4.f);
  }
  Vector3 eye = {g.x, g.y + g.eyeH() + bob, g.z};
  cam.up = {0, 1, 0};
  cam.fovy = st.fov;
  cam.projection = CAMERA_PERSPECTIVE;
  if (viewMode == 0) { cam.position = eye; cam.target = {eye.x + f.x, eye.y + f.y, eye.z + f.z}; return; }
  float sgn = viewMode == 1 ? -1.f : 1.f, t = 0;
  while (t < 4.f && !binfo(g.w.at((int)floorf(eye.x + sgn * f.x * (t + .3f)), (int)floorf(eye.y + sgn * f.y * (t + .3f)), (int)floorf(eye.z + sgn * f.z * (t + .3f)))).solid) t += .2f;
  cam.position = {eye.x + sgn * f.x * t, eye.y + sgn * f.y * t, eye.z + sgn * f.z * t};
  cam.target = eye;
}
static Color lerpC(Color a, Color b, float t) {
  return {(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t), (unsigned char)(a.b + (b.b - a.b) * t), 255};
}
static void skyColors(Color& top, Color& bot, float& bright) {
  if (g.w.dim == NETHER) { top = {40, 8, 8, 255}; bot = {90, 25, 20, 255}; bright = .9f; return; }
  if (g.w.dim == END) { top = {6, 4, 14, 255}; bot = {24, 16, 40, 255}; bright = .75f; return; }
  float d = g.daylight(), s = sinf(g.time / DAYLEN * 6.2831853f), dusk = std::max(0.f, 1 - fabsf(s) / .25f);
  Color dayT = st.vibrant ? Color{50, 140, 255, 255} : Color{95, 160, 225, 255}, dayB = st.vibrant ? Color{150, 215, 255, 255} : Color{160, 205, 240, 255};
  Color nT = {4, 6, 20, 255}, nB = {14, 18, 40, 255}, du = st.vibrant ? Color{255, 120, 60, 255} : Color{232, 133, 90, 255};
  top = lerpC(nT, dayT, d);
  bot = lerpC(nB, dayB, d);
  bot = lerpC(bot, du, dusk * (st.vibrant ? .75f : .55f));
  bright = .26f + .74f * d;
  if (g.weather) {  // overcast / storm
    float k = g.weather == 2 ? .55f : .25f;
    top = lerpC(top, Color{55, 60, 70, 255}, k); bot = lerpC(bot, Color{85, 90, 100, 255}, k);
    bright *= g.weather == 2 ? .7f : .85f;
  }
}
static void drawSky() {
  Color t, b;
  float br;
  skyColors(t, b, br);
  int sw = GetScreenWidth(), sh = GetScreenHeight();
  DrawRectangleGradientV(0, 0, sw, sh, t, b);
  if (g.w.dim != OVERWORLD) return;
  float a = g.time / DAYLEN * 6.2831853f;
  Vector3 f = fwdVec();
  for (int k = 0; k < 2; k++) {  // sun, moon
    float sg = k ? -1.f : 1.f;
    Vector3 d = {cosf(a) * sg, sinf(a) * sg, .25f};
    if (d.x * f.x + d.y * f.y + d.z * f.z < .2f) continue;
    Vector2 p = GetWorldToScreen({cam.position.x + d.x * 100, cam.position.y + d.y * 100, cam.position.z + d.z * 100}, cam);
    float R = sh * .05f;
    if (k == 0) {
      DrawCircle((int)p.x, (int)p.y, R * 1.8f, Fade(Color{255, 230, 150, 255}, st.vibrant ? .25f : .12f));
      DrawRectangle((int)(p.x - R), (int)(p.y - R), (int)(R * 2), (int)(R * 2), Color{255, 244, 190, 255});
    } else DrawRectangle((int)(p.x - R * .8f), (int)(p.y - R * .8f), (int)(R * 1.6f), (int)(R * 1.6f), Color{214, 222, 240, 255});
  }
}
static std::vector<std::pair<int, int>> ring;
static int ringR = -1, ringCx = 1 << 30, ringCz = 1 << 30;

static void drawWorld() {
  int R = rdChunks(st.rd), pcx = (int)floorf(g.x / CS), pcz = (int)floorf(g.z / CS);
  if (R != ringR || pcx != ringCx || pcz != ringCz) {
    ringR = R; ringCx = pcx; ringCz = pcz;
    ring.clear();
    std::vector<std::pair<int, std::pair<int, int>>> tmp;
    for (int dx = -R; dx <= R; dx++)
      for (int dz = -R; dz <= R; dz++)
        if (dx * dx + dz * dz <= R * R + 1) tmp.push_back({dx * dx + dz * dz, {pcx + dx, pcz + dz}});
    std::sort(tmp.begin(), tmp.end());
    for (auto& t : tmp) ring.push_back(t.second);
    g.w.unloadFar(pcx, pcz, R + 2);
    for (auto it = meshes.begin(); it != meshes.end();) {
      if (!g.w.loaded(ckx(it->first), ckz(it->first))) { freeCM(it->second); it = meshes.erase(it); } else ++it;
    }
  }
  double t0 = GetTime(), budget = mobile ? .006 : .009;  // spread chunk generation/meshing over several frames
  for (auto& c : ring) {
    if (GetTime() - t0 > budget) break;
    auto it = meshes.find(ckey(c.first, c.second));
    if (it == meshes.end() || !g.w.loaded(c.first, c.second) || it->second.ver != g.w.chunk(c.first, c.second).ver) buildChunk(c.first, c.second);
  }
  Color tc, bc;
  float br;
  skyColors(tc, bc, br);
  bool nether = g.w.dim == NETHER;
  Color tint = {(unsigned char)(255 * br * (nether ? 1.f : .85f + .15f * g.daylight())), (unsigned char)(255 * br * (nether ? .75f : 1.f)), (unsigned char)(255 * br * (nether ? .7f : 1.f)), 255};
  mat.maps[MATERIAL_MAP_DIFFUSE].texture = st.vibrant ? atlasV : atlasN;
  mat.maps[MATERIAL_MAP_DIFFUSE].color = tint;
  Vector3 f = fwdVec();
  float fl = hypotf(f.x, f.z) + 1e-5f, fx = f.x / fl, fz = f.z / fl;
  float halfH = atanf(tanf(st.fov * DEG2RAD / 2) * GetScreenWidth() / (float)GetScreenHeight());
  std::vector<CM*> vis;
  for (auto& c : ring) {
    auto it = meshes.find(ckey(c.first, c.second));
    if (it == meshes.end()) continue;
    float dx = c.first * CS + 8 - cam.position.x, dz = c.second * CS + 8 - cam.position.z, d = hypotf(dx, dz);
    if (d > 26 && viewMode == 0) {  // frustum culling by view angle
      float ang = acosf(std::max(-1.f, std::min(1.f, (dx * fx + dz * fz) / d)));
      if (ang > halfH + atanf(13.f / d) + .12f) continue;
    }
    vis.push_back(&it->second);
  }
  for (CM* cm : vis) for (auto& m : cm->op) DrawMesh(m, mat, MatrixIdentity());
  for (CM* cm : vis) for (auto& m : cm->tr) DrawMesh(m, mat, MatrixIdentity());
}

// ---------------------------------------------------------------- player model (third person)
static void box(float x, float y, float z, float w, float h, float d, Color c) { DrawCube({x, y, z}, w, h, d, c); }
static void drawPlayerModel(float px, float py, float pz, float yaw, float walk, bool moving, const Skin& sk, const Stack& held, float swing) {
  rlPushMatrix();
  rlTranslatef(px, py, pz);
  rlRotatef(-yaw * RAD2DEG, 0, 1, 0);
  float s = moving ? sinf(walk * 2.6f) * .28f : 0;
  box(-.13f, .35f, s, .24f, .7f, .26f, sk.pants);
  box(.13f, .35f, -s, .24f, .7f, .26f, sk.pants);
  box(0, 1.05f, 0, .52f, .75f, .3f, sk.shirt);
  box(-.38f, 1.05f, -s * .9f, .2f, .72f, .24f, sk.shirt);
  box(.38f, 1.05f, s * .9f + swing * .3f, .2f, .72f, .24f, sk.shirt);
  box(0, 1.65f, 0, .46f, .46f, .46f, sk.skin);
  box(0, 1.84f, -.02f, .48f, .12f, .5f, sk.hair);
  box(-.1f, 1.68f, .24f, .08f, .08f, .02f, BLACK);
  box(.1f, 1.68f, .24f, .08f, .08f, .02f, BLACK);
  if (!held.empty()) {
    Color hc = {255, 255, 255, 255};
    if (isTool(held.id)) { Px p = tierColor(toolTier(held.id)); hc = {(unsigned char)pr(p), (unsigned char)pg(p), (unsigned char)pb(p), 255}; }
    else if (held.id < BLK_COUNT) { Tex t = makeTile(binfo(held.id).side); Px p = t.p[136]; hc = {(unsigned char)pr(p), (unsigned char)pg(p), (unsigned char)pb(p), 255}; }
    box(.38f, .72f, .3f + swing * .2f, .14f, .14f, .4f, hc);
  }
  rlPopMatrix();
}

// ---------------------------------------------------------------- mobs
static Color mixC(Color a, Color b, float t) { return lerpC(a, b, std::max(0.f, std::min(1.f, t))); }
static void drawMob(const Mob& m) {
  float s = std::hypot(m.vx, m.vz) > .3f ? sinf(m.anim * 3.2f) * .28f : 0.f;
  Color red = {255, 60, 60, 255};
  auto C = [&](Color c) { return m.hurt > 0 ? mixC(c, red, .55f) : c; };
  rlPushMatrix();
  rlTranslatef(m.x, m.y, m.z);
  rlRotatef(-m.yaw * RAD2DEG, 0, 1, 0);
  switch (m.type) {
    case M_COW: case M_PIG: case M_SHEEP: case M_CHICKEN: {
      Color bc, hc, lc;
      float L, Hh, Wd, legH;
      if (m.type == M_COW) { bc = {110, 75, 45, 255}; hc = {95, 65, 40, 255}; lc = {80, 55, 35, 255}; L = .9f; Hh = .55f; Wd = .6f; legH = .55f; }
      else if (m.type == M_PIG) { bc = {240, 160, 160, 255}; hc = {235, 150, 150, 255}; lc = {225, 140, 140, 255}; L = .8f; Hh = .45f; Wd = .55f; legH = .35f; }
      else if (m.type == M_SHEEP) { bc = {235, 235, 235, 255}; hc = {70, 60, 55, 255}; lc = {200, 200, 200, 255}; L = .85f; Hh = .6f; Wd = .65f; legH = .5f; }
      else { bc = {245, 245, 245, 255}; hc = {245, 245, 245, 255}; lc = {230, 170, 40, 255}; L = .4f; Hh = .35f; Wd = .3f; legH = .25f; }
      float y = legH + Hh / 2;
      box(0, y, 0, Wd, Hh, L, C(bc));
      box(0, y + Hh * .25f, L / 2 + .15f, Wd * .8f, Hh * .8f, .3f, C(hc));
      if (m.type == M_CHICKEN) { box(0, y + Hh * .2f, L / 2 + .33f, .08f, .06f, .12f, Color{240, 180, 40, 255}); box(0, y + Hh * .62f, L / 2 + .15f, .06f, .1f, .12f, Color{210, 40, 40, 255}); }
      if (m.type == M_PIG) box(0, y + Hh * .15f, L / 2 + .33f, Wd * .4f, .15f, .08f, Color{250, 190, 190, 255});
      for (int i = 0; i < 4; i++) box((i % 2 ? .3f : -.3f) * Wd * 1.1f, legH / 2, (i < 2 ? .3f : -.3f) * L + ((i % 3 == 0) ? s : -s) * .3f, .16f, legH, .16f, C(lc));
      break;
    }
    case M_ZOMBIE: case M_SKELETON: case M_CINDER: {
      Color skin = m.type == M_ZOMBIE ? Color{95, 150, 80, 255} : m.type == M_SKELETON ? Color{220, 220, 210, 255} : Color{150, 50, 40, 255};
      Color shirt = m.type == M_ZOMBIE ? Color{50, 90, 150, 255} : m.type == M_SKELETON ? Color{200, 200, 190, 255} : Color{90, 30, 25, 255};
      Color pants = m.type == M_ZOMBIE ? Color{60, 50, 130, 255} : m.type == M_SKELETON ? Color{190, 190, 180, 255} : Color{60, 25, 20, 255};
      float lw = m.type == M_SKELETON ? .14f : .24f;
      box(-.13f, .35f, s, lw, .7f, lw, C(pants)); box(.13f, .35f, -s, lw, .7f, lw, C(pants));
      box(0, 1.05f, 0, m.type == M_SKELETON ? .4f : .52f, .75f, .3f, C(shirt));
      float arm = m.type == M_SKELETON ? .12f : .2f;
      if (m.type == M_SKELETON) { box(-.3f, 1.05f, .1f, arm, .72f, arm, C(skin)); box(.3f, 1.05f, .25f, arm, .72f, arm, C(skin)); }
      else { box(-.38f, 1.2f, .35f, arm, .2f, .7f, C(skin)); box(.38f, 1.2f, .35f, arm, .2f, .7f, C(skin)); }
      box(0, 1.65f, 0, .46f, .46f, .46f, C(skin));
      box(-.1f, 1.68f, .24f, .08f, .08f, .02f, m.type == M_CINDER ? Color{255, 200, 60, 255} : BLACK); box(.1f, 1.68f, .24f, .08f, .08f, .02f, m.type == M_CINDER ? Color{255, 200, 60, 255} : BLACK);
      break;
    }
    case M_SPIDER: {
      Color bc = {45, 38, 38, 255};
      box(0, .45f, 0, .7f, .4f, .8f, C(bc)); box(0, .5f, .6f, .5f, .35f, .45f, C(Color{60, 50, 50, 255}));
      box(-.1f, .55f, .85f, .09f, .09f, .02f, Color{230, 40, 40, 255}); box(.1f, .55f, .85f, .09f, .09f, .02f, Color{230, 40, 40, 255});
      for (int i = 0; i < 4; i++) for (int sd = -1; sd <= 1; sd += 2) box(sd * .6f, .25f, -.3f + i * .3f + ((i + sd) % 2 ? s : -s) * .3f, .6f, .08f, .08f, C(bc));
      break;
    }
    case M_CREEPER: {
      bool flash = m.fuse >= 0 && ((int)(m.fuse * 10) % 2 == 0);
      Color g1 = flash ? Color{255, 255, 255, 255} : C(Color{80, 170, 80, 255}), g2 = flash ? Color{255, 255, 255, 255} : C(Color{60, 140, 60, 255});
      box(0, .9f, 0, .5f, .9f, .3f, g1); box(0, 1.55f, 0, .5f, .5f, .5f, g1);
      box(-.1f, 1.6f, .26f, .1f, .12f, .02f, BLACK); box(.1f, 1.6f, .26f, .1f, .12f, .02f, BLACK); box(0, 1.45f, .26f, .12f, .15f, .02f, BLACK);
      for (int i = 0; i < 4; i++) box((i % 2 ? .15f : -.15f), .22f, (i < 2 ? .18f : -.18f) + ((i % 3 == 0) ? s : -s) * .3f, .2f, .44f, .2f, g2);
      break;
    }
    case M_VILLAGER: {
      static const Color rb[6] = {{110, 140, 70, 255}, {235, 235, 235, 255}, {70, 70, 78, 255}, {120, 125, 135, 255}, {120, 70, 150, 255}, {60, 130, 130, 255}};
      Color robe = rb[m.prof % 6], skin = {222, 175, 135, 255};
      box(-.13f, .3f, s, .25f, .6f, .27f, C(robe)); box(.13f, .3f, -s, .25f, .6f, .27f, C(robe));
      box(0, 1.0f, 0, .6f, .8f, .36f, C(robe)); box(0, 1.1f, .22f, .62f, .22f, .26f, C(skin));
      box(0, 1.62f, 0, .46f, .5f, .46f, C(skin)); box(0, 1.55f, .27f, .12f, .24f, .14f, C(Color{200, 150, 115, 255}));
      box(0, 1.82f, 0, .5f, .08f, .5f, C(Color{70, 50, 35, 255}));
      break;
    }
    case M_WARPER: {
      Color k = {22, 18, 28, 255};
      box(-.1f, .6f, s, .14f, 1.2f, .14f, C(k)); box(.1f, .6f, -s, .14f, 1.2f, .14f, C(k));
      box(0, 1.65f, 0, .5f, .9f, .26f, C(k));
      box(-.36f, 1.5f, -s * .8f, .1f, 1.3f, .12f, C(k)); box(.36f, 1.5f, s * .8f, .1f, 1.3f, .12f, C(k));
      box(0, 2.4f, 0, .44f, .44f, .44f, C(k));
      box(-.1f, 2.42f, .23f, .09f, .05f, .02f, Color{220, 90, 255, 255}); box(.1f, 2.42f, .23f, .09f, .05f, .02f, Color{220, 90, 255, 255});
      break;
    }
    case M_DRAGON: {
      Color b1 = {38, 30, 52, 255}, b2 = {60, 46, 84, 255};
      float flap = sinf(m.anim * 4.f) * 28.f;
      box(0, 1, 0, 1.6f, 1.1f, 3.4f, C(b1)); box(0, 1.15f, 2.3f, .9f, .8f, 1.5f, C(b2)); box(0, 1.0f, 3.2f, .5f, .4f, .7f, C(b1));
      box(0, 1.0f, -2.2f, .8f, .6f, 1.6f, C(b1)); box(0, .9f, -3.6f, .5f, .4f, 1.4f, C(b2));
      box(-.3f, 1.4f, 2.6f, .14f, .14f, .04f, Color{255, 80, 200, 255}); box(.3f, 1.4f, 2.6f, .14f, .14f, .04f, Color{255, 80, 200, 255});
      for (int sd = -1; sd <= 1; sd += 2) { rlPushMatrix(); rlTranslatef(sd * .8f, 1.5f, 0); rlRotatef(sd * flap, 0, 0, 1); box(sd * 2.4f, 0, 0, 4.8f, .12f, 2.6f, C(b2)); rlPopMatrix(); }
      break;
    }
    case M_CRYSTAL: {
      rlRotatef(m.anim * 80.f, 0, 1, 0);
      float bob = sinf(m.anim * 2.f) * .12f;
      box(0, .4f + bob, 0, .55f, .55f, .55f, C(Color{230, 90, 230, 255})); box(0, .4f + bob, 0, .3f, .9f, .3f, C(Color{255, 200, 255, 255}));
      break;
    }
    default: break;
  }
  rlPopMatrix();
}

// ---------------------------------------------------------------- UI helpers
static float uiS() { return std::max(1.f, GetScreenHeight() / 360.f); }
static bool inRect(Vector2 m, float x, float y, float w, float h) { return m.x >= x && m.x < x + w && m.y >= y && m.y < y + h; }
static bool btn(float x, float y, float w, float h, const char* t, bool en = true, bool on = false) {
  Vector2 m = GetMousePosition();
  bool in = en && inRect(m, x, y, w, h);
  Color c = !en ? Color{50, 50, 50, 255} : in ? Color{104, 150, 78, 255} : on ? Color{90, 130, 160, 255} : Color{68, 104, 58, 255};
  DrawRectangle((int)x, (int)y, (int)w, (int)h, Color{10, 15, 8, 255});
  DrawRectangle((int)x + 3, (int)y + 3, (int)w - 6, (int)h - 9, c);
  DrawRectangle((int)x + 3, (int)(y + h - 6), (int)w - 6, 3, Fade(BLACK, .4f));
  int fs = std::max(8, (int)(h * .42f));
  DrawText(t, (int)(x + (w - MeasureText(t, fs)) / 2), (int)(y + (h - 6 - fs) / 2 + 3), fs, en ? RAYWHITE : GRAY);
  return in && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}
static void shadowText(const char* t, int x, int y, int fs, Color c) { DrawText(t, x + 1, y + 1, fs, Color{0, 0, 0, 200}); DrawText(t, x, y, fs, c); }
static void centerText(const char* t, float cx, float y, int fs, Color c) { shadowText(t, (int)(cx - MeasureText(t, fs) / 2), (int)y, fs, c); }
static void dimScreen(float a) { DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, a)); }

static void drawStack(float x, float y, float s, const Stack& k) {
  if (k.empty()) return;
  Rectangle src = {(float)(k.id % 16 * 16), (float)(k.id / 16 * 16), 16, 16};
  DrawTexturePro(iconTex, src, {x + s * .1f, y + s * .1f, s * .8f, s * .8f}, {0, 0}, 0, WHITE);
  if (k.n > 1) {
    const char* t = TextFormat("%d", k.n);
    int fs = std::max(8, (int)(s * .3f));
    shadowText(t, (int)(x + s * .94f - MeasureText(t, fs)), (int)(y + s * .64f), fs, WHITE);
  }
  if (maxDur(k.id) > 0 && k.dur < maxDur(k.id)) {  // durability bar
    float fr = (float)k.dur / maxDur(k.id);
    DrawRectangle((int)(x + s * .12f), (int)(y + s * .86f), (int)(s * .76f), std::max(2, (int)(s * .07f)), BLACK);
    DrawRectangle((int)(x + s * .12f), (int)(y + s * .86f), (int)(s * .76f * fr), std::max(2, (int)(s * .07f)), lerpC({220, 50, 40, 255}, {80, 220, 60, 255}, fr));
  }
}

// ---------------------------------------------------------------- HUD
static void pixelHeart(float x, float y, float s, int fill) {  // 0 empty, 1 half, 2 full
  static const char* H[6] = {".XX.XX.", "XXXXXXX", "XXXXXXX", ".XXXXX.", "..XXX..", "...X..."};
  for (int r = 0; r < 6; r++)
    for (int c = 0; c < 7; c++)
      if (H[r][c] == 'X') {
        bool full = fill == 2 || (fill == 1 && c < 4);
        DrawRectangle((int)(x + c * s), (int)(y + r * s), (int)ceilf(s), (int)ceilf(s), full ? Color{220, 40, 40, 255} : Color{50, 20, 20, 255});
      }
}
static void pixelFood(float x, float y, float s, int fill) {
  Color mc = fill == 2 ? Color{205, 120, 60, 255} : fill == 1 ? Color{160, 100, 60, 255} : Color{60, 45, 35, 255};
  DrawRectangle((int)(x + 1 * s), (int)(y + 0 * s), (int)(4 * s), (int)(4 * s), mc);
  DrawRectangle((int)(x + 0 * s), (int)(y + 1 * s), (int)(6 * s), (int)(2 * s), mc);
  DrawRectangle((int)(x + 4 * s), (int)(y + 4 * s), (int)(2 * s), (int)(2 * s), Color{230, 225, 210, 255});
  DrawRectangle((int)(x + 5 * s), (int)(y + 5 * s), (int)(2 * s), (int)(2 * s), Color{230, 225, 210, 255});
}
static void drawWeather() {
  int sw = GetScreenWidth(), sh = GetScreenHeight();
  if (g.flash > 0) DrawRectangle(0, 0, sw, sh, Fade(WHITE, g.flash * .55f));
  if (g.strikeShow > 0) {  // lightning bolt
    Vector3 f = fwdVec();
    if ((g.strikeX - cam.position.x) * f.x + (g.strikeZ - cam.position.z) * f.z > 1) {
      Vector2 a = GetWorldToScreen({g.strikeX, 90, g.strikeZ}, cam), b = GetWorldToScreen({g.strikeX, g.y, g.strikeZ}, cam), prev = a;
      for (int i = 1; i <= 7; i++) {
        float t = i / 7.f;
        Vector2 p = {a.x + (b.x - a.x) * t + ((i % 2) ? 14.f : -14.f) * (i < 7 ? 1.f : 0.f), a.y + (b.y - a.y) * t};
        DrawLineEx(prev, p, 3, Fade(Color{235, 235, 255, 255}, .9f));
        prev = p;
      }
    }
  }
  if (g.weather == 0 || g.w.dim != OVERWORLD) return;
  int pc = g.precip(g.x, g.z);
  if (pc == 0 || !g.skyOpen((int)floorf(g.x), (int)floorf(g.y + 1), (int)floorf(g.z))) return;  // none in deserts or under a roof
  double t = GetTime();
  int n = g.weather == 2 ? 260 : 160;
  for (int i = 0; i < n; i++) {
    float rx = fmodf(i * 97.31f + 13.f, (float)sw), speed = pc == 2 ? 90.f : 700.f + (i % 5) * 60.f, ry = fmodf(i * 53.17f + (float)t * speed, (float)sh);
    if (pc == 2) DrawRectangle((int)(rx + sinf((float)t + i) * 12), (int)ry, 3, 3, Fade(WHITE, .85f));
    else DrawLine((int)rx, (int)ry, (int)rx - 3, (int)ry + 16, Fade(Color{170, 190, 230, 255}, .55f));
  }
}
static void pixelShield(float x, float y, float s, int fill) {
  static const char* H[7] = {"XXXXXXX", "XXXXXXX", "XXXXXXX", "XXXXXXX", ".XXXXX.", "..XXX..", "...X..."};
  for (int r = 0; r < 7; r++)
    for (int c = 0; c < 7; c++)
      if (H[r][c] == 'X') DrawRectangle((int)(x + c * s), (int)(y + r * s), (int)ceilf(s), (int)ceilf(s), (fill == 2 || (fill == 1 && c < 4)) ? Color{190, 200, 215, 255} : Color{70, 75, 85, 255});
}
static float hotS() { return std::max(28.f, std::min(GetScreenWidth() / 11.5f, 36.f * uiS())); }
static Rectangle hotRect(int i) {
  float s = hotS(), w = 9 * s;
  return {(GetScreenWidth() - w) / 2 + i * s, GetScreenHeight() - s - 4 * uiS(), s, s};
}
static int lastSel = -1;
static float selFade = 0;
static void drawHUD(float dt) {
  int sw = GetScreenWidth(), sh = GetScreenHeight();
  float s = hotS();
  { Vector3 f = fwdVec();  // health bars over hurt mobs
    for (auto& m : g.mobs) {
      const MobInfo& I = minfo(m.type);
      if (m.hp >= I.hp) continue;
      Vector3 d = {m.x - cam.position.x, m.y + I.h + .4f - cam.position.y, m.z - cam.position.z};
      if (d.x * f.x + d.y * f.y + d.z * f.z < 1) continue;
      Vector2 p = GetWorldToScreen({m.x, m.y + I.h + .4f, m.z}, cam);
      float bw = 34;
      DrawRectangle((int)(p.x - bw / 2), (int)p.y, (int)bw, 5, Fade(BLACK, .7f));
      DrawRectangle((int)(p.x - bw / 2) + 1, (int)p.y + 1, (int)((bw - 2) * std::max(0.f, m.hp) / I.hp), 3, I.hostile ? Color{220, 60, 50, 255} : Color{90, 200, 90, 255});
    } }
  for (auto& m : g.mobs)
    if (m.type == M_DRAGON) {  // boss bar
      float bw2 = sw * .5f, fr = std::max(0.f, m.hp) / minfo(M_DRAGON).hp, bx = (sw - bw2) / 2;
      DrawRectangle((int)bx - 2, 28, (int)bw2 + 4, 14, Fade(BLACK, .7f));
      DrawRectangle((int)bx, 30, (int)(bw2 * fr), 10, Color{190, 60, 220, 255});
      centerText("SKY WYRM", sw / 2.f, 12, 14, WHITE);
    }
  if (g.noteT > 0 && !g.note.empty()) centerText(g.note.c_str(), sw / 2.f, sh * .12f, (int)(14 * uiS() / 1.4f), Fade(Color{255, 235, 140, 255}, std::min(1.f, g.noteT)));
  if (viewMode == 0 && !g.locked) {
    DrawRectangle(sw / 2 - 1, sh / 2 - 9, 2, 18, Fade(WHITE, .85f));
    DrawRectangle(sw / 2 - 9, sh / 2 - 1, 18, 2, Fade(WHITE, .85f));
  }
  float hx = (sw - 9 * s) / 2, hy = hotRect(0).y;
  DrawRectangle((int)hx - 4, (int)hy - 4, (int)(9 * s) + 8, (int)s + 8, Fade(BLACK, .55f));
  for (int i = 0; i < 9; i++) {
    Rectangle r = hotRect(i);
    DrawRectangle((int)r.x + 1, (int)r.y + 1, (int)s - 2, (int)s - 2, Fade(Color{70, 70, 70, 255}, .7f));
    DrawRectangleLines((int)r.x, (int)r.y, (int)s, (int)s, i == g.sel ? WHITE : Color{20, 20, 20, 255});
    if (i == g.sel) DrawRectangleLines((int)r.x + 1, (int)r.y + 1, (int)s - 2, (int)s - 2, WHITE);
    drawStack(r.x, r.y, s, g.inv.s[i]);
  }
  if (lastSel != g.sel) { lastSel = g.sel; selFade = 2; }
  selFade = std::max(0.f, selFade - dt);
  if (selFade > 0 && !g.held().empty()) centerText(iname(g.held().id).c_str(), sw / 2.f, hy - s * .75f - 20, (int)(s * .36f), Fade(WHITE, std::min(1.f, selFade)));
  if (g.mode != CREATIVE && !g.locked) {
    float hs = std::max(2.f, s / 8.f), y0 = hy - 7 * hs - 12;
    int hp = (int)ceilf(g.v.hp);
    for (int i = 0; i < 10; i++) pixelHeart(hx + i * 8 * hs, y0, hs, hp >= (i + 1) * 2 ? 2 : hp == i * 2 + 1 ? 1 : 0);
    int ap = g.armorPoints();
    for (int i = 0; i < 10 && ap >= i * 2 + 1; i++) pixelShield(hx + i * 8 * hs, y0 - 9 * hs, hs, ap >= (i + 1) * 2 ? 2 : 1);
    int fd = (int)ceilf(g.v.food);
    for (int i = 0; i < 10; i++) pixelFood(hx + 9 * s - (i + 1) * 8 * hs, y0, hs, fd >= (i + 1) * 2 ? 2 : fd == i * 2 + 1 ? 1 : 0);
    if (g.v.air < 14.9f)
      for (int i = 0; i < (int)ceilf(g.v.air / 1.5f); i++) DrawCircle((int)(hx + 9 * s - (i + 1) * 8 * hs + 3 * hs), (int)(y0 - 18 * hs), hs * 2.2f, Fade(Color{150, 200, 255, 255}, .85f));
  }
  if (g.v.hurtT > 0) DrawRectangle(0, 0, sw, sh, Fade(RED, std::min(.4f, g.v.hurtT * .8f)));
  drawWeather();
  if (g.headWet) DrawRectangle(0, 0, sw, sh, Fade(Color{20, 70, 160, 255}, .3f));
  if (g.portalT > 0) DrawRectangle(0, 0, sw, sh, Fade(Color{140, 60, 220, 255}, std::min(.65f, g.portalT / 3.5f * .65f)));
  if (g.locked) centerText("SPECTATOR - this hardcore world is locked", sw / 2.f, 10, (int)(14 * uiS() / 1.4f), Color{255, 120, 120, 255});
  if (g.eatT > 0) {
    DrawRectangle(sw / 2 - 40, sh / 2 + 24, 80, 6, Fade(BLACK, .6f));
    DrawRectangle(sw / 2 - 40, sh / 2 + 24, (int)(80 * g.eatT / 1.6f), 6, Color{220, 160, 60, 255});
  }
  if (g.mode == CREATIVE) shadowText(g.flying ? "CREATIVE - flying" : "CREATIVE", 8, 8, 12, Fade(WHITE, .8f));
  if (debug) {
    int bi = g.w.dim == OVERWORLD ? biomeAt(g.w.seed, (int)g.x, (int)g.z, terrainH(g.w.seed, (int)g.x, (int)g.z)) : 0;
    shadowText(TextFormat("%d FPS  xyz %.1f %.1f %.1f  %s  chunks %d  meshes %d  rd %d", GetFPS(), g.x, g.y, g.z, g.w.dim == OVERWORLD ? biomeName(bi) : "-", (int)g.w.ch.size(), (int)meshes.size(), rdChunks(st.rd)), 8, 26, 12, YELLOW);
  }
  if (toastT > 0) {
    centerText(toast.c_str(), sw / 2.f, sh * .2f, (int)(16 * uiS() / 1.4f), Fade(YELLOW, std::min(1.f, toastT)));
    toastT -= dt;
  }
}

// ---------------------------------------------------------------- inventory / crafting / furnace UI
static Stack heldS, cgrid[9];
static int invMode = 0;  // 0 inventory (2x2), 1 crafting table (3x3), 2 furnace
static int curFx = 0, curFy = 0, curFz = 0, palPage = 0, bookScroll = 0;
static bool splitMode = false, bookOpen = false, onlyCraft = false;
static Stack* hoverStack = nullptr;

static void giveBack(const Stack& k) {
  if (k.empty()) return;
  int left = g.inv.add(k.id, k.n, k.dur);
  if (left > 0) {
    Pile p; p.x = g.x; p.y = g.y; p.z = g.z; p.dim = g.w.dim;
    Stack r = k; r.n = left;
    p.items.push_back(r);
    g.piles.push_back(p);
  }
}
static void openInv(int m) { invMode = m; scr = S_INV; heldS = Stack(); bookScroll = 0; }
static void closeInv() {
  for (auto& c : cgrid) { giveBack(c); c = Stack(); }
  giveBack(heldS);
  heldS = Stack();
  scr = S_PLAY;
}
static int slotUI(float x, float y, float s, Stack& k) {
  Vector2 m = GetMousePosition();
  bool in = inRect(m, x, y, s, s);
  DrawRectangle((int)x, (int)y, (int)s, (int)s, Color{20, 20, 20, 255});
  DrawRectangle((int)x + 1, (int)y + 1, (int)s - 2, (int)s - 2, in ? Color{95, 95, 100, 255} : Color{62, 62, 66, 255});
  drawStack(x, y, s, k);
  if (in) hoverStack = &k;
  if (in && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return splitMode ? 2 : 1;
  if (in && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) return 2;
  return 0;
}
static void slotClick(Stack& k, int r) {
  if (!r) return;
  if (r == 1) {  // left: pick up / place / merge / swap
    if (heldS.empty()) { heldS = k; k = Stack(); }
    else if (k.empty()) { k = heldS; heldS = Stack(); }
    else if (k.id == heldS.id && maxStack(k.id) > 1) {
      int a = std::min(heldS.n, maxStack(k.id) - k.n);
      k.n += a; heldS.n -= a;
      if (heldS.n <= 0) heldS = Stack();
    } else std::swap(k, heldS);
  } else {  // right: split half / place one
    if (heldS.empty()) {
      if (!k.empty()) { int h = (k.n + 1) / 2; heldS = k; heldS.n = h; k.n -= h; if (k.n <= 0) k = Stack(); }
    } else if (k.empty() || (k.id == heldS.id && k.n < maxStack(k.id) && maxStack(k.id) > 1)) {
      if (k.empty()) { k = heldS; k.n = 1; } else k.n++;
      if (--heldS.n <= 0) heldS = Stack();
    }
  }
}
static void bookText(const Rec& r, std::string& out, bool& ok) {
  std::vector<std::pair<int, int>> need;
  recNeeds(r, need);
  out.clear();
  ok = true;
  for (auto& p : need) {
    int have = g.inv.count(p.first);
    if (have < p.second) ok = false;
    out += iname(p.first) + " " + std::to_string(std::min(have, p.second)) + "/" + std::to_string(p.second) + "   ";
  }
}
static std::vector<int> palette() {
  static std::vector<int> p;
  if (p.empty()) {
    for (int id = 1; id < I_END; id++) if (validItem(id)) p.push_back(id);
  }
  return p;
}
static void drawInventory() {
  int sw = GetScreenWidth(), sh = GetScreenHeight();
  dimScreen(.6f);
  hoverStack = nullptr;
  bool land = sw >= sh * 1.5f, creative = g.mode == CREATIVE;
  bool showBook = bookOpen && !creative && invMode != 2 && invMode != 3;
  float S = std::floor(std::min(sh / 9.4f, sw / (showBook && land ? 15.2f : 9.8f)));
  S = std::max(S, 22.f);
  float totalW = 9 * S + (showBook && land ? 5.6f * S : 0);
  float ox = (sw - totalW) / 2, oy = (sh - 8.6f * S) / 2;
  DrawRectangle((int)(ox - S * .3f), (int)(oy - S * .15f), (int)(9 * S + S * .6f), (int)(8.5f * S), Color{198, 198, 198, 255});
  DrawRectangle((int)(ox - S * .3f) + 3, (int)(oy - S * .15f) + 3, (int)(9 * S + S * .6f) - 6, (int)(8.5f * S) - 6, Color{139, 139, 139, 255});
  float bh = S * .62f;
  const char* title = invMode == 3 ? "Chest" : creative ? "Creative Items" : invMode == 1 ? "Crafting Table" : invMode == 2 ? "Furnace" : "Crafting";
  DrawText(title, (int)ox, (int)(oy + bh * .1f), (int)(S * .42f), Color{40, 40, 40, 255});
  float y1 = oy + S * .75f, y2 = y1 + 3.25f * S, y3 = y2 + 3.2f * S;
  if (btn(ox + 9 * S - 2.4f * S, oy - S * .05f, 2.4f * S, bh, "CLOSE")) { closeInv(); return; }
  if (btn(ox + 9 * S - 4.7f * S, oy - S * .05f, 2.2f * S, bh, splitMode ? "SPLIT ON" : "SPLIT", true, splitMode)) splitMode = !splitMode;
  if (!creative && invMode != 2 && invMode != 3 && btn(ox + 9 * S - 7.3f * S, oy - S * .05f, 2.5f * S, bh, "RECIPES", true, bookOpen)) bookOpen = !bookOpen;
  bool hideMain = showBook && !land;
  if (!hideMain) {
    if (invMode == 3) {  // chest: 27 slots above your inventory
      ChestData* cd = g.chestAt(curFx, curFy, curFz, true, true);
      if (!cd || g.w.at(curFx, curFy, curFz) != CHEST) { closeInv(); return; }
      for (int i = 0; i < 27; i++) slotClick(cd->s[i], slotUI(ox + (i % 9) * S, y1 + (i / 9) * S, S, cd->s[i]));
    } else if (creative) {  // creative item palette: click an item to put a full stack in the selected hotbar slot
      std::vector<int> pal = palette();
      int per = 27, pages = ((int)pal.size() + per - 1) / per;
      palPage = std::max(0, std::min(pages - 1, palPage));
      for (int i = 0; i < per; i++) {
        int k = palPage * per + i;
        Stack tmp = k < (int)pal.size() ? mk(pal[k], 1) : Stack();
        if (slotUI(ox + (i % 9) * S, y1 + (i / 9) * S, S, tmp) && k < (int)pal.size()) g.held() = mk(pal[k], maxStack(pal[k]));
      }
      if (btn(ox, y1 + 3 * S + 2, 1.5f * S, S * .3f, "<", palPage > 0)) palPage--;
      if (btn(ox + 7.5f * S, y1 + 3 * S + 2, 1.5f * S, S * .3f, ">", palPage < pages - 1)) palPage++;
    } else if (invMode == 2) {  // furnace
      Furn* f = g.furnAt(curFx, curFy, curFz);
      if (!f) { closeInv(); return; }
      slotClick(f->in, slotUI(ox + 2 * S, y1, S, f->in));
      slotClick(f->fuel, slotUI(ox + 2 * S, y1 + 2 * S, S, f->fuel));
      float fl = f->burnMax > 0 ? f->burn / f->burnMax : 0;
      DrawRectangle((int)(ox + 2.25f * S), (int)(y1 + 1.1f * S), (int)(S * .5f), (int)(S * .8f), Color{50, 50, 50, 255});
      if (fl > 0) DrawRectangle((int)(ox + 2.25f * S), (int)(y1 + 1.1f * S + S * .8f * (1 - fl)), (int)(S * .5f), (int)(S * .8f * fl), Color{255, (unsigned char)(120 + 80 * sinf((float)GetTime() * 14)), 30, 255});
      DrawRectangle((int)(ox + 3.4f * S), (int)(y1 + 1.35f * S), (int)(S * 1.4f), (int)(S * .3f), Color{50, 50, 50, 255});
      DrawRectangle((int)(ox + 3.4f * S), (int)(y1 + 1.35f * S), (int)(S * 1.4f * f->prog / 10.f), (int)(S * .3f), Color{250, 250, 250, 255});
      int r = slotUI(ox + 5.4f * S, y1 + S, S, f->out);
      if (r && !f->out.empty() && (heldS.empty() || (heldS.id == f->out.id && heldS.n + f->out.n <= maxStack(heldS.id)))) {
        if (heldS.empty()) heldS = f->out; else heldS.n += f->out.n;
        f->out = Stack();
      }
      centerText("Fuel: coal, logs, planks, sticks", ox + 4.5f * S, y1 + 2.7f * S, (int)(S * .26f), Color{50, 50, 50, 255});
    } else {  // crafting grid (2x2 in inventory, 3x3 at a table) + armor slots
      float asz = S * .78f;
      for (int i = 0; i < 4; i++) {
        int r = slotUI(ox, y1 + i * asz, asz, g.armor[i]);
        if (r && (heldS.empty() || (isArmor(heldS.id) && armorPiece(heldS.id) == i))) slotClick(g.armor[i], r);
        if (g.armor[i].empty()) { static const char* lb[4] = {"H", "C", "L", "B"}; DrawText(lb[i], (int)(ox + asz * .38f), (int)(y1 + i * asz + asz * .25f), (int)(asz * .5f), Color{100, 100, 105, 255}); }
      }
      int gw = invMode == 1 ? 3 : 2;
      float gx = invMode == 1 ? ox + 1.0f * S : ox + 1.6f * S, gy = invMode == 1 ? y1 : y1 + .5f * S;
      for (int yy = 0; yy < gw; yy++)
        for (int xx = 0; xx < gw; xx++) slotClick(cgrid[yy * gw + xx], slotUI(gx + xx * S, gy + yy * S, S, cgrid[yy * gw + xx]));
      int on = 0, oid = matchGrid(cgrid, gw, gw, &on);
      DrawRectangle((int)(ox + 4.0f * S), (int)(y1 + 1.35f * S), (int)(S * 1.0f), (int)(S * .3f), Color{80, 80, 80, 255});
      Stack outS = oid ? mk(oid, on) : Stack();
      int r = slotUI(ox + 5.4f * S, y1 + S, S, outS);
      if (r && oid && (heldS.empty() || (heldS.id == oid && maxStack(oid) > 1 && heldS.n + on <= maxStack(oid)))) {
        if (heldS.empty()) heldS = outS; else heldS.n += on;
        for (int i = 0; i < gw * gw; i++) if (!cgrid[i].empty() && --cgrid[i].n <= 0) cgrid[i] = Stack();
      }
    }
    for (int i = 0; i < 27; i++) slotClick(g.inv.s[9 + i], slotUI(ox + (i % 9) * S, y2 + (i / 9) * S, S, g.inv.s[9 + i]));
    for (int i = 0; i < 9; i++) slotClick(g.inv.s[i], slotUI(ox + i * S, y3, S, g.inv.s[i]));
  }
  if (showBook) {  // recipe book: CRAFT builds the item straight from your inventory
    float bx = land ? ox + 9.5f * S : ox, by = land ? oy : y1, bw = land ? 5.1f * S : 9 * S, bhh = land ? 8.3f * S : 7.6f * S;
    DrawRectangle((int)bx, (int)by, (int)bw, (int)bhh, Color{198, 198, 198, 255});
    DrawRectangle((int)bx + 3, (int)by + 3, (int)bw - 6, (int)bhh - 6, Color{120, 120, 124, 255});
    if (btn(bx + S * .1f, by + S * .1f, bw - S * .2f, S * .5f, onlyCraft ? "Showing: craftable" : "Showing: all", true, onlyCraft)) onlyCraft = !onlyCraft;
    std::vector<int> rows;
    const auto& R = recipes();
    for (size_t i = 0; i < R.size(); i++) {
      std::string t;
      bool ok;
      bookText(R[i], t, ok);
      if (!onlyCraft || (ok && (!R[i].table() || invMode == 1))) rows.push_back((int)i);
    }
    float rh = S * 1.05f, area = bhh - S * 1.2f;
    int vis = std::max(1, (int)(area / rh));
    float wheel = GetMouseWheelMove();
    if (inRect(GetMousePosition(), bx, by, bw, bhh) && wheel != 0) bookScroll -= (int)wheel;
    bookScroll = std::max(0, std::min(std::max(0, (int)rows.size() - vis), bookScroll));
    for (int k = 0; k < vis && bookScroll + k < (int)rows.size(); k++) {
      const Rec& r = R[rows[bookScroll + k]];
      float ry = by + S * .75f + k * rh;
      std::string t;
      bool ok;
      bookText(r, t, ok);
      bool can = ok && (!r.table() || invMode == 1);
      DrawRectangle((int)(bx + S * .1f), (int)ry, (int)(bw - S * .2f), (int)(rh - 3), can ? Color{90, 130, 90, 255} : Color{85, 85, 90, 255});
      drawStack(bx + S * .1f, ry + (rh - S) / 2, S, mk(r.out, r.n));
      std::string nm = iname(r.out) + (r.n > 1 ? " x" + std::to_string(r.n) : "") + (r.table() ? "  (table)" : "");
      DrawText(nm.c_str(), (int)(bx + S * 1.1f), (int)(ry + 2), (int)(S * .27f), WHITE);
      DrawText(t.c_str(), (int)(bx + S * 1.1f), (int)(ry + S * .42f), (int)(S * .2f), ok ? Color{210, 240, 210, 255} : Color{255, 170, 150, 255});
      if (btn(bx + bw - S * 1.9f, ry + rh * .15f, S * 1.7f, rh * .62f, "CRAFT", can)) {
        if (autoCraft(g.inv, r)) say("Crafted " + iname(r.out), 1.2f); else say("Not enough room");
      }
      if (ok && r.table() && invMode != 1) DrawText("needs a crafting table", (int)(bx + S * 1.1f), (int)(ry + S * .66f), (int)(S * .2f), Color{255, 220, 120, 255});
    }
  }
  if (!heldS.empty()) {
    Vector2 m = GetMousePosition();
    drawStack(m.x - S / 2, m.y - S / 2, S, heldS);
  } else if (hoverStack && !hoverStack->empty()) {
    Vector2 m = GetMousePosition();
    const Stack& k = *hoverStack;
    std::string t = iname(k.id);
    std::string sub = maxDur(k.id) > 0 ? "Durability " + std::to_string(k.dur) + "/" + std::to_string(maxDur(k.id))
                      : foodValue(k.id) ? "Food: restores " + std::to_string(foodValue(k.id)) + " hunger"
                      : smeltOf(k.id) ? "Can be smelted in a furnace" : "";
    int fs = (int)(S * .3f), w = std::max(MeasureText(t.c_str(), fs), MeasureText(sub.c_str(), (int)(fs * .8f))) + 12;
    float tx = std::min(m.x + 14, (float)sw - w - 4), ty = m.y + 14, th = fs * (sub.empty() ? 1.8f : 3.0f);
    DrawRectangle((int)tx, (int)ty, w, (int)th, Color{20, 8, 30, 235});
    DrawRectangleLines((int)tx, (int)ty, w, (int)th, Color{90, 40, 150, 255});
    DrawText(t.c_str(), (int)tx + 6, (int)ty + 4, fs, WHITE);
    if (!sub.empty()) DrawText(sub.c_str(), (int)tx + 6, (int)ty + 6 + fs, (int)(fs * .8f), Color{170, 170, 170, 255});
  }
}

// ---------------------------------------------------------------- villager trading
static int tradeMob = -1;
static void drawTrade() {
  int sw = GetScreenWidth(), sh = GetScreenHeight();
  dimScreen(.6f);
  if (tradeMob < 0 || tradeMob >= (int)g.mobs.size() || g.mobs[tradeMob].type != M_VILLAGER) { scr = S_PLAY; return; }
  Mob& m = g.mobs[tradeMob];
  const auto& T = trades(m.prof);
  float S = std::max(26.f, std::floor(std::min(sh / 9.f, sw / 11.f)));
  float w = std::min((float)sw - 10, 11.f * S), x0 = (sw - w) / 2, y0 = sh * .05f, h = S * (T.size() * 1.25f + 2.5f);
  DrawRectangle((int)x0, (int)y0, (int)w, (int)h, Color{198, 198, 198, 255});
  DrawRectangle((int)x0 + 3, (int)y0 + 3, (int)w - 6, (int)h - 6, Color{139, 139, 139, 255});
  DrawText((std::string(profName(m.prof)) + " Villager").c_str(), (int)(x0 + S * .3f), (int)(y0 + S * .25f), (int)(S * .48f), Color{40, 40, 40, 255});
  drawStack(x0 + w - S * 2.6f, y0 + S * .1f, S, mk(I_EMERALD, std::max(1, g.inv.count(I_EMERALD))));
  DrawText(TextFormat("x%d", g.inv.count(I_EMERALD)), (int)(x0 + w - S * 1.6f), (int)(y0 + S * .4f), (int)(S * .4f), Color{20, 90, 40, 255});
  for (size_t i = 0; i < T.size(); i++) {
    const Trade& t = T[i];
    float ry = y0 + S * (1.2f + i * 1.25f);
    bool left = m.uses[i] < t.max, can = left && g.inv.count(t.give) >= t.giveN;
    DrawRectangle((int)(x0 + S * .2f), (int)ry, (int)(w - S * .4f), (int)(S * 1.15f), can ? Color{110, 150, 110, 255} : Color{105, 105, 110, 255});
    drawStack(x0 + S * .3f, ry + S * .08f, S, mk(t.give, t.giveN));
    DrawText("->", (int)(x0 + S * 1.45f), (int)(ry + S * .35f), (int)(S * .45f), WHITE);
    drawStack(x0 + S * 2.1f, ry + S * .08f, S, mk(t.get, t.getN));
    DrawText((iname(t.give) + " x" + std::to_string(t.giveN) + " > " + iname(t.get) + " x" + std::to_string(t.getN)).c_str(), (int)(x0 + S * 3.2f), (int)(ry + S * .12f), (int)(S * .26f), WHITE);
    DrawText(left ? TextFormat("%d trades left", t.max - m.uses[i]) : "Sold out", (int)(x0 + S * 3.2f), (int)(ry + S * .62f), (int)(S * .24f), left ? Color{225, 235, 200, 255} : Color{255, 170, 150, 255});
    if (btn(x0 + w - S * 2.5f, ry + S * .15f, S * 2.2f, S * .85f, "TRADE", can)) { if (g.doTrade(tradeMob, (int)i)) say("Traded!", 1.2f); }
  }
  if (btn(x0 + w - S * 2.7f, y0 + h - S * 1.15f, S * 2.5f, S * .8f, "CLOSE")) scr = S_PLAY;
  if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ESCAPE)) scr = S_PLAY;
}

// ---------------------------------------------------------------- menus
static std::string fName, fSeed;
static int focusF = 0, cMode = 0, cDiff = 2, worldScroll = 0;
static std::string confirmDel;
static std::vector<Meta> worldList;

static void startDemo() {
  g = Game();
  g.newWorld("demo", 20260, CREATIVE, 0);
  g.id = "";
  g.w.lastC = nullptr;
  clearMeshes();
  ringR = -1;
}
static void enterGame(Screen to) {
  g.w.lastC = nullptr;
  g.w.lastK = ~0ull;
  clearMeshes();
  ringR = -1;
  heldS = Stack();
  scr = to;
}
static void startNew(const std::string& nm, uint32_t seed, int mode, int diff) {
  g = Game();
  g.newWorld(nm, seed, mode, diff);
  g.id = "w" + std::to_string((long long)time(nullptr)) + std::to_string(GetRandomValue(0, 9999));
  g.played = time(nullptr);
  saveGame(g);
  enterGame(S_PLAY);
}
static bool startLoad(const std::string& id) {
  Game t;
  if (!loadGame(id, t)) { say("Could not load that world"); return false; }
  g = std::move(t);
  enterGame(g.dead ? S_DEAD : S_PLAY);
  return true;
}
static void doSave(bool show = true) {
  if (g.id.empty()) return;
  g.played = time(nullptr);
  if (show) say("Saving World...", 1.2f);
  if (!saveGame(g)) say("Save failed!", 3);
}
static uint32_t seedOf(const std::string& s) {
  if (s.empty()) return (uint32_t)GetRandomValue(1, 2000000000);
  bool digits = true;
  for (char c : s) if (!isdigit((unsigned char)c)) digits = false;
  if (digits && s.size() < 10) return (uint32_t)std::stoul(s);
  uint32_t h = 7;
  for (char c : s) h = h * 31 + (uint32_t)c;
  return h;
}
static void typeInto(std::string& s, size_t maxLen) {
  int c;
  while ((c = GetCharPressed()) > 0) if (c >= 32 && c < 127 && s.size() < maxLen) s.push_back((char)c);
  if (IsKeyPressed(KEY_BACKSPACE) && !s.empty()) s.pop_back();
}
static void field(float x, float y, float w, float h, std::string& s, int id, const char* hint) {
  bool in = inRect(GetMousePosition(), x, y, w, h);
  if (in && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) focusF = id;
  DrawRectangle((int)x, (int)y, (int)w, (int)h, focusF == id ? Color{200, 200, 255, 255} : Color{10, 10, 10, 255});
  DrawRectangle((int)x + 2, (int)y + 2, (int)w - 4, (int)h - 4, Color{25, 25, 30, 255});
  int fs = (int)(h * .45f);
  if (s.empty()) DrawText(hint, (int)x + 8, (int)(y + h * .27f), fs, Color{110, 110, 110, 255});
  else DrawText(s.c_str(), (int)x + 8, (int)(y + h * .27f), fs, WHITE);
  if (focusF == id && ((int)(GetTime() * 2) % 2 == 0)) DrawText("_", (int)x + 8 + MeasureText(s.c_str(), fs), (int)(y + h * .27f), fs, WHITE);
  if (focusF == id) typeInto(s, 24);
}
static const char* MODES[3] = {"Survival", "Creative", "Hardcore"};
static const char* DIFFS[4] = {"Peaceful", "Easy", "Normal", "Hard"};

static void drawSkinPreview(float x, float y, float s, const Skin& k) {
  DrawRectangle((int)(x + s * 1.5f), (int)y, (int)(s * 2), (int)(s * 2), k.skin);
  DrawRectangle((int)(x + s * 1.5f), (int)y, (int)(s * 2), (int)(s * .5f), k.hair);
  DrawRectangle((int)(x + s * 1.9f), (int)(y + s * .9f), (int)(s * .25f), (int)(s * .25f), BLACK);
  DrawRectangle((int)(x + s * 2.8f), (int)(y + s * .9f), (int)(s * .25f), (int)(s * .25f), BLACK);
  DrawRectangle((int)(x + s * 1.5f), (int)(y + s * 2), (int)(s * 2), (int)(s * 3), k.shirt);
  DrawRectangle((int)(x + s * .5f), (int)(y + s * 2), (int)s, (int)(s * 3), k.shirt);
  DrawRectangle((int)(x + s * 4.0f), (int)(y + s * 2), (int)s, (int)(s * 3), k.shirt);
  DrawRectangle((int)(x + s * 1.5f), (int)(y + s * 5), (int)s, (int)(s * 3), k.pants);
  DrawRectangle((int)(x + s * 2.5f), (int)(y + s * 5), (int)s, (int)(s * 3), k.pants);
}

static void drawMenus() {
  int sw = GetScreenWidth(), sh = GetScreenHeight();
  float u = uiS(), bw = std::min(sw * .86f, 300.f * u / 1.4f), bh = std::min(sh * .085f, 34.f * u / 1.2f), gap = bh * .25f, cx = sw / 2.f;
  if (scr == S_TITLE) {
    dimScreen(.35f);
    int tf = (int)(sh * .16f);
    float tw = (float)(MeasureText("VOXEL", tf) + MeasureText("3D", tf)), tx = cx - tw / 2, ty = sh * .07f;
    shadowText("VOXEL", (int)tx, (int)ty, tf, Color{240, 240, 230, 255});
    shadowText("3D", (int)tx + MeasureText("VOXEL", tf), (int)ty, tf, Color{127, 191, 74, 255});
    float y = sh * .30f;
    auto next = [&]() { float r = y; y += bh + gap; return r; };
    if (btn(cx - bw / 2, next(), bw, bh, "PLAY")) {
      worldList = listWorlds();
      if (!worldList.empty()) startLoad(worldList[0].id);
      else { scr = S_CREATE; fName = ""; fSeed = ""; }
    }
    if (btn(cx - bw / 2, next(), bw, bh, "CREATE WORLD")) { scr = S_CREATE; fName = ""; fSeed = ""; focusF = 0; }
    if (btn(cx - bw / 2, next(), bw, bh, "WORLD LIST")) { worldList = listWorlds(); worldScroll = 0; confirmDel.clear(); scr = S_WORLDS; }
    if (btn(cx - bw / 2, next(), bw, bh, "SETTINGS")) { prevScr = S_TITLE; scr = S_SETTINGS; }
    if (btn(cx - bw / 2, next(), bw, bh, "SKINS")) scr = S_SKINS;
    if (btn(cx - bw / 2, next(), bw, bh, "CONTROLS")) { prevScr = S_TITLE; scr = S_CONTROLS; }
    if (btn(cx - bw / 2, next(), bw, bh, "QUIT")) quitting = true;
  } else if (scr == S_CREATE) {
    dimScreen(.55f);
    centerText("CREATE NEW WORLD", cx, sh * .06f, (int)(sh * .07f), WHITE);
    float y = sh * .17f;
    auto next = [&]() { float r = y; y += bh + gap; return r; };
    if (!mobile) {
      field(cx - bw / 2, next(), bw, bh, fName, 1, "World name");
      field(cx - bw / 2, next(), bw, bh, fSeed, 2, "Seed (blank = random)");
    } else { DrawText("Name and seed are generated automatically on touch devices", (int)(cx - bw / 2), (int)y, (int)(bh * .32f), LIGHTGRAY); y += bh * 1.2f; }
    if (btn(cx - bw / 2, next(), bw, bh, TextFormat("Game Mode: %s", MODES[cMode]))) cMode = (cMode + 1) % 3;
    if (btn(cx - bw / 2, next(), bw, bh, TextFormat("Difficulty: %s", cMode == HARDCORE ? "Hard (locked)" : DIFFS[cDiff]), cMode != HARDCORE)) cDiff = (cDiff + 1) % 4;
    const char* desc = cMode == 0 ? "Empty inventory. Gather, craft, survive." : cMode == 1 ? "Unlimited items, flying, no damage." : "One life. Death locks the world forever.";
    centerText(desc, cx, y, (int)(bh * .34f), Color{230, 220, 150, 255});
    y += bh * .7f;
    if (btn(cx - bw / 2, next(), bw, bh, "GENERATE WORLD")) {
      size_t n = listWorlds().size() + 1;
      startNew(fName.empty() ? "World " + std::to_string(n) : fName, seedOf(fSeed), cMode, cDiff);
    }
    if (btn(cx - bw / 2, next(), bw, bh, "BACK")) scr = S_TITLE;
  } else if (scr == S_WORLDS) {
    dimScreen(.6f);
    centerText("WORLDS", cx, sh * .04f, (int)(sh * .07f), WHITE);
    float rowH = std::max(bh * 1.6f, 52.f), top = sh * .15f, area = sh * .72f;
    int vis = std::max(1, (int)(area / rowH));
    worldScroll = std::max(0, std::min(std::max(0, (int)worldList.size() - vis), worldScroll - (int)GetMouseWheelMove()));
    float w = std::min(sw * .94f, 560.f * u / 1.4f);
    if (worldList.empty()) centerText("No worlds yet. Create one!", cx, top + 20, (int)(bh * .5f), LIGHTGRAY);
    for (int k = 0; k < vis && worldScroll + k < (int)worldList.size(); k++) {
      const Meta& m = worldList[worldScroll + k];
      float y = top + k * rowH, x = cx - w / 2;
      DrawRectangle((int)x, (int)y, (int)w, (int)(rowH - 4), Color{30, 36, 28, 235});
      DrawRectangleLines((int)x, (int)y, (int)w, (int)(rowH - 4), Color{90, 110, 80, 255});
      DrawText(m.name.c_str(), (int)x + 8, (int)y + 4, (int)(rowH * .3f), WHITE);
      char when[64] = "";
      time_t t = (time_t)m.played;
      std::strftime(when, sizeof when, "%b %d %Y %H:%M", std::localtime(&t));
      DrawText(TextFormat("%s%s | Seed %lld | %s | %s", MODES[m.mode % 3], m.dead ? " (DEAD)" : "", m.seed, m.dim == 0 ? "Overworld" : m.dim == 1 ? "Nether" : "End", when), (int)x + 8, (int)(y + rowH * .5f), (int)(rowH * .2f), Color{190, 200, 180, 255});
      float bwid = std::min(w * .2f, 90.f * u / 1.4f);
      if (btn(x + w - bwid * 2 - 8, y + 4, bwid, rowH - 12, "PLAY")) { startLoad(m.id); return; }
      bool sure = confirmDel == m.id;
      if (btn(x + w - bwid - 4, y + 4, bwid, rowH - 12, sure ? "SURE?" : "DELETE")) {
        if (sure) { deleteWorld(m.id); confirmDel.clear(); worldList = listWorlds(); return; }
        confirmDel = m.id;
      }
    }
    if (btn(cx - bw / 2, sh - bh * 1.3f, bw, bh, "BACK")) scr = S_TITLE;
  } else if (scr == S_SETTINGS) {
    dimScreen(.6f);
    centerText("SETTINGS", cx, sh * .04f, (int)(sh * .07f), WHITE);
    float y = sh * .14f;
    auto next = [&]() { float r = y; y += bh + gap * .8f; return r; };
    static const char* RD[4] = {"Low", "Medium", "High", "Ultra"};
    int R = rdChunks(st.rd);
    if (btn(cx - bw / 2, next(), bw, bh, TextFormat("Render Distance: %s (%d chunks, ~%d drawn)", RD[st.rd], R, (int)(3.14f * R * R)))) { st.rd = (st.rd + 1) % 4; saveSettings(); }
    if (btn(cx - bw / 2, next(), bw, bh, TextFormat("Vibrant Visuals: %s", st.vibrant ? "ON" : "OFF"), true, st.vibrant)) { st.vibrant = !st.vibrant; clearMeshes(); saveSettings(); }
    if (btn(cx - bw / 2, next(), bw, bh, TextFormat("View Bobbing: %s", st.bob ? "ON" : "OFF"), true, st.bob)) { st.bob = !st.bob; saveSettings(); }
    auto slider = [&](const char* nm, float& v, float lo, float hi, float stp, const char* fmt) {
      float yy = next();
      if (btn(cx - bw / 2, yy, bh, bh, "-")) { v = std::max(lo, v - stp); saveSettings(); }
      if (btn(cx + bw / 2 - bh, yy, bh, bh, "+")) { v = std::min(hi, v + stp); saveSettings(); }
      centerText(TextFormat(fmt, nm, v), cx, yy + bh * .28f, (int)(bh * .4f), WHITE);
    };
    slider("FOV", st.fov, 50, 110, 5, "%s: %.0f");
    slider("Mouse Sensitivity", st.sens, .2f, 3, .1f, "%s: %.1f");
    slider("Touch Sensitivity", st.tsens, .2f, 3, .1f, "%s: %.1f");
    slider("Volume", st.vol, 0, 1, .1f, "%s: %.1f");
    if (btn(cx - bw / 2, next(), bw, bh, "BACK")) scr = prevScr == S_PAUSE ? S_PAUSE : S_TITLE;
  } else if (scr == S_SKINS) {
    dimScreen(.6f);
    centerText("SKINS", cx, sh * .04f, (int)(sh * .07f), WHITE);
    float cell = std::min(sw / 3.4f, sh / 3.1f), x0 = cx - cell * 1.5f, y0 = sh * .13f;
    for (int i = 0; i < 6; i++) {
      float x = x0 + (i % 3) * cell, y = y0 + (i / 3) * cell * 1.05f;
      bool on = st.skin == i;
      DrawRectangle((int)x + 4, (int)y + 4, (int)cell - 8, (int)cell - 8, on ? Color{80, 120, 70, 255} : Color{35, 40, 33, 235});
      drawSkinPreview(x + cell * .12f, y + cell * .08f, cell * .105f, SKINS[i]);
      DrawText(SKINS[i].name, (int)(x + cell / 2 - MeasureText(SKINS[i].name, (int)(cell * .1f)) / 2), (int)(y + cell * .86f), (int)(cell * .1f), WHITE);
      if (inRect(GetMousePosition(), x, y, cell, cell) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { st.skin = i; saveSettings(); }
    }
    centerText("Visible in third-person view (F5 or VIEW button)", cx, sh - bh * 2.0f, (int)(bh * .36f), LIGHTGRAY);
    if (btn(cx - bw / 2, sh - bh * 1.3f, bw, bh, "BACK")) scr = S_TITLE;
  } else if (scr == S_CONTROLS) {
    dimScreen(.65f);
    centerText("CONTROLS", cx, sh * .04f, (int)(sh * .07f), WHITE);
    static const char* L[] = {
        "WASD move   Mouse look   Space jump (double-tap = fly in Creative)",
        "Shift sprint   C or Ctrl sneak / fly down   Left click mine   Right click place / use / eat (hold)",
        "1-9 or mouse wheel: hotbar   E inventory   F5 camera mode   F3 debug   Esc pause",
        "Right-click: Crafting Table, Furnace, wear armor, bucket, bow (needs arrows), flint and steel on obsidian = nether portal.",
        "Touch: left thumb = move stick (push to the edge to sprint), drag right side = look",
        "MINE (hold)  PLACE (tap, hold to eat)  JUMP  SNEAK  FLY  BAG  VIEW  II",
        "Survival: punch trees > planks > sticks > crafting table > pickaxe > stone > ores > furnace > better tools.",
        "Pickaxe tiers: stone needs wood, iron ore needs stone, gold and diamond need iron, obsidian needs diamond.",
        "Hunger drains when sprinting and healing. Eat meat (cook it in a furnace), apples. Monsters spawn at night and in the dark.",
        "Portal: 4x5 obsidian frame (2x3 opening), light it with flint (from gravel) and steel. Water on lava makes obsidian; mine it with a diamond pickaxe.",
        "Villagers: right-click to trade emeralds. Warp Pearl + Ember Rod = Seeker Eye: use it to find a stronghold, fill its 12 frames, enter the End.",
        "Levers, buttons and plates + circuit wire power lamps and doors. Torches keep monsters away. Right-click chests. Hold right-click with a shield to block."};
    float y = sh * .15f;
    int fs = std::max(10, (int)(sh * .033f));
    for (const char* l : L) { centerText(l, cx, y, fs, Color{225, 225, 215, 255}); y += fs * 1.55f; }
    if (btn(cx - bw / 2, sh - bh * 1.3f, bw, bh, "BACK")) scr = prevScr == S_PAUSE ? S_PAUSE : S_TITLE;
  } else if (scr == S_PAUSE) {
    dimScreen(.55f);
    centerText("GAME PAUSED", cx, sh * .12f, (int)(sh * .08f), WHITE);
    float y = sh * .28f;
    auto next = [&]() { float r = y; y += bh + gap; return r; };
    if (btn(cx - bw / 2, next(), bw, bh, "RESUME")) scr = S_PLAY;
    if (btn(cx - bw / 2, next(), bw, bh, "SAVE")) doSave();
    if (btn(cx - bw / 2, next(), bw, bh, "SETTINGS")) { prevScr = S_PAUSE; scr = S_SETTINGS; }
    if (btn(cx - bw / 2, next(), bw, bh, "CONTROLS")) { prevScr = S_PAUSE; scr = S_CONTROLS; }
    if (btn(cx - bw / 2, next(), bw, bh, "EXIT TO TITLE")) { doSave(false); startDemo(); scr = S_TITLE; }
  } else if (scr == S_DEAD) {
    DrawRectangle(0, 0, sw, sh, Fade(g.mode == HARDCORE ? Color{60, 0, 0, 255} : Color{120, 0, 0, 255}, .6f));
    centerText(g.mode == HARDCORE ? "GAME OVER" : "YOU DIED", cx, sh * .15f, (int)(sh * .12f), WHITE);
    if (g.mode == HARDCORE) centerText("Hardcore: this world is now permanently locked.", cx, sh * .3f, (int)(sh * .045f), Color{255, 200, 200, 255});
    else centerText("Your items are in a pile where you died.", cx, sh * .3f, (int)(sh * .045f), Color{255, 230, 200, 255});
    float y = sh * .42f;
    auto next = [&]() { float r = y; y += bh + gap; return r; };
    if (btn(cx - bw / 2, next(), bw, bh, g.mode == HARDCORE ? "SPECTATE WORLD" : "RESPAWN")) { g.respawn(); scr = S_PLAY; doSave(false); }
    if (btn(cx - bw / 2, next(), bw, bh, "EXIT TO TITLE")) { if (g.mode == HARDCORE) g.locked = true; doSave(false); startDemo(); scr = S_TITLE; }
  }
}

// ---------------------------------------------------------------- touch controls
struct TBtn { float x, y, r; const char* label; };
static int joyId = -1;
static Vector2 joyO{0, 0}, joyP{0, 0};
static std::unordered_map<int, Vector2> tLast;
static std::set<int> tPrev;
static bool sneakLatch = false;

static void touchButtons(TBtn* b) {
  float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight(), R = std::min(sh * .09f, 54.f);
  b[0] = {sw - 2.0f * R, sh - 2.4f * R, 1.2f * R, "MINE"};
  b[1] = {sw - 4.5f * R, sh - 1.7f * R, 1.0f * R, "PLACE"};
  b[2] = {sw - 1.7f * R, sh - 5.0f * R, 1.0f * R, "JUMP"};
  b[3] = {sw - 4.4f * R, sh - 4.0f * R, .8f * R, "SNEAK"};
  b[4] = {sw - 6.6f * R, sh - 2.4f * R, .8f * R, "FLY"};
  b[5] = {sw - 1.1f * R, 1.1f * R, .75f * R, "BAG"};
  b[6] = {sw - 2.9f * R, 1.1f * R, .75f * R, "VIEW"};
  b[7] = {sw - 4.7f * R, 1.1f * R, .75f * R, "II"};
}
static void drawTouch() {
  TBtn b[8];
  touchButtons(b);
  for (int i = 0; i < 8; i++) {
    if (i == 4 && g.mode != CREATIVE) continue;
    bool on = (i == 3 && sneakLatch) || (i == 4 && g.flying);
    DrawCircle((int)b[i].x, (int)b[i].y, b[i].r, Fade(on ? Color{120, 200, 120, 255} : WHITE, .22f));
    DrawCircleLines((int)b[i].x, (int)b[i].y, b[i].r, Fade(WHITE, .6f));
    int fs = (int)(b[i].r * .42f);
    DrawText(b[i].label, (int)(b[i].x - MeasureText(b[i].label, fs) / 2), (int)(b[i].y - fs / 2), fs, WHITE);
  }
  if (joyId >= 0) {
    float R = std::min(GetScreenHeight() * .09f, 54.f) * 1.6f;
    Vector2 d = {joyP.x - joyO.x, joyP.y - joyO.y};
    float l = hypotf(d.x, d.y);
    if (l > R) { d.x *= R / l; d.y *= R / l; }
    DrawCircle((int)joyO.x, (int)joyO.y, R, Fade(WHITE, .12f));
    DrawCircleLines((int)joyO.x, (int)joyO.y, R, Fade(WHITE, .5f));
    DrawCircle((int)(joyO.x + d.x), (int)(joyO.y + d.y), R * .4f, Fade(WHITE, .45f));
  } else DrawText("MOVE", (int)(GetScreenWidth() * .06f), (int)(GetScreenHeight() * .62f), 14, Fade(WHITE, .4f));
}

// ---------------------------------------------------------------- play update
static float lastSpace = -1;
static void playUpdate(float dt) {
  Input in;
  int sw = GetScreenWidth(), sh = GetScreenHeight();
  bool mine = false, usePress = false, useHold = false, tJump = false, attackPress = false;
  if (!mobile) {
    in.fwd = (IsKeyDown(KEY_W) ? 1.f : 0.f) - (IsKeyDown(KEY_S) ? 1.f : 0.f);
    in.right = (IsKeyDown(KEY_D) ? 1.f : 0.f) - (IsKeyDown(KEY_A) ? 1.f : 0.f);
    in.jump = IsKeyDown(KEY_SPACE);
    in.sprint = IsKeyDown(KEY_LEFT_SHIFT);
    in.sneak = IsKeyDown(KEY_C) || IsKeyDown(KEY_LEFT_CONTROL);
    if (IsKeyPressed(KEY_SPACE)) {
      float t = (float)GetTime();
      if (t - lastSpace < .3f && (g.mode == CREATIVE || g.locked)) g.flying = !g.flying;
      lastSpace = t;
    }
    Vector2 md = GetMouseDelta();
    g.yaw += md.x * .0025f * st.sens;
    g.pitch = std::max(-1.55f, std::min(1.55f, g.pitch - md.y * .0025f * st.sens));
    mine = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    attackPress = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    usePress = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    useHold = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    for (int k = 0; k < 9; k++) if (IsKeyPressed(KEY_ONE + k)) g.sel = k;
    float wh = GetMouseWheelMove();
    if (wh != 0) g.sel = ((g.sel + (wh > 0 ? -1 : 1)) % 9 + 9) % 9;
    if (IsKeyPressed(KEY_E)) { openInv(0); return; }
    if (IsKeyPressed(KEY_F5)) viewMode = (viewMode + 1) % 3;
    if (IsKeyPressed(KEY_F3)) debug = !debug;
    if (IsKeyPressed(KEY_ESCAPE)) { scr = S_PAUSE; doSave(false); return; }
    if (IsKeyPressed(KEY_P) && g.mode == CREATIVE) {  // creative-only dimension hop; survival uses portals (planned)
      g.changeDim((g.w.dim + 1) % 3);
      clearMeshes();
      ringR = -1;
      doSave(false);
      say(g.w.dim == 0 ? "Overworld" : g.w.dim == 1 ? "The Nether" : "The End");
    }
  } else {
    TBtn b[8];
    touchButtons(b);
    std::set<int> cur;
    Vector2 jv{0, 0};
    float R = std::min(sh * .09f, 54.f);
    for (int i = 0; i < GetTouchPointCount(); i++) {
      Vector2 p = GetTouchPosition(i);
      int id = GetTouchPointId(i);
      cur.insert(id);
      bool isNew = !tPrev.count(id);
      int hit = -1;
      for (int k = 0; k < 8; k++) if (hypotf(p.x - b[k].x, p.y - b[k].y) <= b[k].r * 1.15f && !(k == 4 && g.mode != CREATIVE)) hit = k;
      if (id == joyId) { joyP = p; continue; }
      if (hit >= 0) {
        if (hit == 0) { mine = true; if (isNew) attackPress = true; }
        if (hit == 1) { useHold = true; if (isNew) usePress = true; }
        if (hit == 2) tJump = true;
        if (isNew) {
          if (hit == 3) sneakLatch = !sneakLatch;
          if (hit == 4) g.flying = !g.flying;
          if (hit == 5) { tPrev = cur; openInv(0); return; }
          if (hit == 6) viewMode = (viewMode + 1) % 3;
          if (hit == 7) { tPrev = cur; scr = S_PAUSE; doSave(false); return; }
        }
        continue;
      }
      if (isNew) {
        bool onHot = false;
        for (int k = 0; k < 9; k++) { Rectangle r = hotRect(k); if (inRect(p, r.x, r.y, r.width, r.height)) { g.sel = k; onHot = true; } }
        if (onHot) continue;
        if (joyId < 0 && p.x < sw * .42f && p.y > sh * .28f) { joyId = id; joyO = p; joyP = p; continue; }
      } else if (tLast.count(id)) {
        g.yaw += (p.x - tLast[id].x) * .005f * st.tsens;
        g.pitch = std::max(-1.55f, std::min(1.55f, g.pitch - (p.y - tLast[id].y) * .005f * st.tsens));
      }
      tLast[id] = p;
    }
    if (joyId >= 0 && !cur.count(joyId)) joyId = -1;
    if (joyId >= 0) {
      float lim = R * 1.6f;
      jv = {(joyP.x - joyO.x) / lim, (joyP.y - joyO.y) / lim};
      float l = hypotf(jv.x, jv.y);
      if (l > 1) { jv.x /= l; jv.y /= l; }
      in.sprint = l >= .97f;
    }
    tPrev = cur;
    in.right = jv.x;
    in.fwd = -jv.y;
    in.jump = tJump;
    in.sneak = sneakLatch;
  }
  g.blocking = useHold && !g.held().empty() && g.held().id == I_SHIELD;
  g.update(dt, in);
  Hit h = g.raycast();
  int tmob = g.targetMob();  // a mob under the crosshair takes the hit instead of the block behind it
  if (tmob >= 0) { mine = false; if (attackPress) g.attackMob(tmob); }
  if (g.mineStep(dt, mine, h)) rebuildNear(h.x, h.z);
  bool foodInHand = !g.held().empty() && foodValue(g.held().id) > 0;
  int hid = g.held().empty() ? 0 : g.held().id;
  int vt = g.villagerTarget();
  if (usePress && vt >= 0) { tradeMob = vt; scr = S_TRADE; return; }
  if (usePress && (hid == I_BUCKET || hid == I_WBUCKET || hid == I_LBUCKET)) {
    Hit hf = g.raycast(5.5f, true);
    if (g.useBucket(hf)) rebuildNear(hf.x, hf.z);
  } else if (usePress && hid == I_BOW) {
    g.shoot();
  } else if (usePress && hid == I_EYE) {
    g.useEye(h);
    rebuildNear(h.x, h.z);
  } else if (usePress && !foodInHand) {
    int r = g.use(h);
    if (r == 1 || r == 5) rebuildNear(h.x + h.nx, h.z + h.nz);
    else if (r == 2) { openInv(1); return; }
    else if (r == 3) { curFx = h.x; curFy = h.y; curFz = h.z; openInv(2); return; }
    else if (r == 6) { curFx = h.x; curFy = h.y; curFz = h.z; openInv(3); return; }
    else if (r == 7) rebuildNear(h.x, h.z);
  }
  g.eatStep(dt, useHold && foodInHand);
  if (g.dead) scr = S_DEAD;
  saveTimer += dt;
  if (saveTimer > 120) { saveTimer = 0; doSave(); }  // autosave every 2 minutes
}

// ---------------------------------------------------------------- main
int main() {
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
  InitWindow(1280, 720, "Voxel3D");
  SetExitKey(KEY_NULL);
  SetTargetFPS(60);
#if defined(PLATFORM_ANDROID)
  mobile = true;
  saveRoot() = "/data/data/com.voxel3d.game/files/saves";  // app-private storage (the working directory is read-only on Android)
#endif
  loadSettings();
  buildTextures();
  buildAudio();
  startDemo();
  float orbit = 0;
  while (!WindowShouldClose() && !quitting) {
    float dt = std::min(GetFrameTime(), .05f);
    if (GetTouchPointCount() > 0) mobile = true;
    bool wantOff = scr == S_PLAY && !mobile;
    if (wantOff != cursorOff) { if (wantOff) DisableCursor(); else EnableCursor(); cursorOff = wantOff; }
    if (scr == S_PLAY) playUpdate(dt);
    else if (scr == S_INV) {  // world keeps ticking (furnaces smelt, time passes) while the inventory is open
      g.time = std::fmod(g.time + dt, DAYLEN);
      for (auto& f : g.furn) furnTick(f, dt);
      if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ESCAPE)) closeInv();
    } else if (scr == S_PAUSE) { if (IsKeyPressed(KEY_ESCAPE)) scr = S_PLAY; }
    else if (scr != S_DEAD && scr != S_TRADE) { g.time = std::fmod(g.time + dt * 4, DAYLEN); orbit += dt * .12f; }

    for (int e : g.events) playSfx(e);
    g.events.clear();
    if (audioOK) SetMasterVolume(st.vol);
    bool menuView = !(scr == S_PLAY || scr == S_INV || scr == S_PAUSE || scr == S_DEAD || scr == S_TRADE);
    if (menuView) {  // animated voxel background: camera orbits the demo world
      cam.up = {0, 1, 0};
      cam.fovy = 70;
      cam.projection = CAMERA_PERSPECTIVE;
      cam.position = {g.x + cosf(orbit) * 30, 36, g.z + sinf(orbit) * 30};
      cam.target = {g.x, 22, g.z};
      g.yaw = -orbit - 1.57f;
      g.pitch = -.3f;
    } else updateCamera(dt);

    BeginDrawing();
    ClearBackground(BLACK);
    drawSky();
    BeginMode3D(cam);
    drawWorld();
    for (auto& p : g.piles)
      if (p.dim == g.w.dim) {
        float bob = sinf((float)GetTime() * 3) * .1f;
        DrawCube({p.x, p.y + .5f + bob, p.z}, .35f, .35f, .35f, GOLD);
        DrawCubeWires({p.x, p.y + .5f + bob, p.z}, .37f, .37f, .37f, BROWN);
      }
    for (auto& m : g.mobs) drawMob(m);
    for (auto& a : g.arrows) DrawCube({a.x, a.y, a.z}, .08f, .08f, .08f, Color{200, 190, 160, 255});
    for (auto& e : g.eyes) DrawCube({e.x, e.y, e.z}, .25f, .25f, .25f, Color{80, 230, 140, 255});
    if (!menuView && viewMode != 0) {
      bool moving = std::hypot(g.vx, g.vz) > .4f;
      float swing = g.mActive && IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? sinf((float)GetTime() * 14) : 0;
      Skin sk2 = SKINS[st.skin];  // worn armor recolors the model
      for (int pc = 0; pc < 4; pc++)
        if (!g.armor[pc].empty()) {
          Px ap = armorColor(g.armor[pc].id);
          Color ac = {(unsigned char)pr(ap), (unsigned char)pg(ap), (unsigned char)pb(ap), 255};
          if (pc == 0) sk2.hair = ac; else if (pc == 1) sk2.shirt = ac; else if (pc == 2) sk2.pants = ac;
        }
      drawPlayerModel(g.x, g.y, g.z, g.yaw, g.walkT, moving, sk2, g.held(), swing);
    }
    if (scr == S_PLAY) {
      Hit h = g.raycast();
      if (h.ok && !g.locked) {
        Vector3 c = {h.x + .5f, h.y + .5f, h.z + .5f};
        float p = g.mineProgress(h), need = g.mineTime(h.b);
        if (p > 0 && need > 0) DrawCube(c, 1.006f, 1.006f, 1.006f, Fade(BLACK, std::min(.65f, p / need * .65f)));  // cracking overlay
        DrawCubeWires(c, 1.01f, 1.01f, 1.01f, BLACK);
      }
    }
    EndMode3D();
    if (scr == S_PLAY || scr == S_INV || scr == S_PAUSE || scr == S_DEAD || scr == S_TRADE) {
      drawHUD(dt);
      if (mobile && scr == S_PLAY) drawTouch();
    }
    if (scr == S_INV) drawInventory();
    if (scr == S_TRADE) drawTrade();
    drawMenus();
    if (menuView && toastT > 0) { centerText(toast.c_str(), GetScreenWidth() / 2.f, GetScreenHeight() * .92f, 18, YELLOW); toastT -= dt; }
    EndDrawing();
  }
  if (!g.id.empty() && (scr == S_PLAY || scr == S_INV || scr == S_PAUSE)) doSave(false);
  saveSettings();
  clearMeshes();
  if (audioOK) CloseAudioDevice();
  if (cursorOff) EnableCursor();
  CloseWindow();
  return 0;
}
