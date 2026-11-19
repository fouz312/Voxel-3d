// Minimal declarations of the raylib 5.x API used by main.cpp. ONLY for `g++ -fsyntax-only` checks; never linked.
#pragma once
#include <cmath>
#define DEG2RAD (3.14159265358979323846f / 180.0f)
#define RAD2DEG (180.0f / 3.14159265358979323846f)
typedef struct Vector2 { float x, y; } Vector2;
typedef struct Vector3 { float x, y, z; } Vector3;
typedef struct Matrix { float m0, m4, m8, m12, m1, m5, m9, m13, m2, m6, m10, m14, m3, m7, m11, m15; } Matrix;
typedef struct Color { unsigned char r, g, b, a; } Color;
typedef struct Rectangle { float x, y, width, height; } Rectangle;
typedef struct Image { void* data; int width, height, mipmaps, format; } Image;
typedef struct Texture { unsigned int id; int width, height, mipmaps, format; } Texture;
typedef Texture Texture2D;
typedef struct Shader { unsigned int id; int* locs; } Shader;
typedef struct MaterialMap { Texture2D texture; Color color; float value; } MaterialMap;
typedef struct Material { Shader shader; MaterialMap* maps; float params[4]; } Material;
typedef struct Mesh { int vertexCount, triangleCount; float* vertices; float* texcoords; float* texcoords2; float* normals; float* tangents; unsigned char* colors; unsigned short* indices; float* animVertices; float* animNormals; unsigned char* boneIds; float* boneWeights; unsigned int vaoId; unsigned int* vboId; } Mesh;
typedef struct Camera3D { Vector3 position, target, up; float fovy; int projection; } Camera3D;
typedef Camera3D Camera;
enum { CAMERA_PERSPECTIVE = 0 };
enum { FLAG_MSAA_4X_HINT = 0x20, FLAG_WINDOW_RESIZABLE = 0x04 };
enum { PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 = 7 };
enum { TEXTURE_FILTER_POINT = 0 };
enum { MATERIAL_MAP_DIFFUSE = 0 };
enum { KEY_NULL = 0, KEY_SPACE = 32, KEY_ZERO = 48, KEY_ONE = 49, KEY_A = 65, KEY_C = 67, KEY_D = 68, KEY_E = 69, KEY_P = 80, KEY_S = 83, KEY_W = 87, KEY_ESCAPE = 256, KEY_BACKSPACE = 259, KEY_F3 = 292, KEY_F5 = 294, KEY_LEFT_SHIFT = 340, KEY_LEFT_CONTROL = 341 };
enum { MOUSE_BUTTON_LEFT = 0, MOUSE_BUTTON_RIGHT = 1 };
#define LIGHTGRAY Color{200, 200, 200, 255}
#define GOLD Color{255, 203, 0, 255}
#define BROWN Color{127, 106, 79, 255}
#define RED Color{230, 41, 55, 255}
#define YELLOW Color{253, 249, 0, 255}
#define WHITE Color{255, 255, 255, 255}
#define BLACK Color{0, 0, 0, 255}
#define RAYWHITE Color{245, 245, 245, 255}
#define GRAY Color{130, 130, 130, 255}
void InitWindow(int, int, const char*);
void CloseWindow(void);
bool WindowShouldClose(void);
void SetConfigFlags(unsigned int);
void SetExitKey(int);
void SetTargetFPS(int);
int GetScreenWidth(void);
int GetScreenHeight(void);
float GetFrameTime(void);
double GetTime(void);
int GetFPS(void);
int GetRandomValue(int, int);
void BeginDrawing(void);
void EndDrawing(void);
void ClearBackground(Color);
void BeginMode3D(Camera3D);
void EndMode3D(void);
void DisableCursor(void);
void EnableCursor(void);
bool IsKeyDown(int);
bool IsKeyPressed(int);
int GetCharPressed(void);
Vector2 GetMousePosition(void);
Vector2 GetMouseDelta(void);
float GetMouseWheelMove(void);
bool IsMouseButtonDown(int);
bool IsMouseButtonPressed(int);
int GetTouchPointCount(void);
Vector2 GetTouchPosition(int);
int GetTouchPointId(int);
Color Fade(Color, float);
const char* TextFormat(const char*, ...);
int MeasureText(const char*, int);
void DrawText(const char*, int, int, int, Color);
void DrawRectangle(int, int, int, int, Color);
void DrawRectangleLines(int, int, int, int, Color);
void DrawRectangleGradientV(int, int, int, int, Color, Color);
void DrawCircle(int, int, float, Color);
void DrawCircleLines(int, int, float, Color);
void DrawCube(Vector3, float, float, float, Color);
void DrawCubeWires(Vector3, float, float, float, Color);
void DrawTexturePro(Texture2D, Rectangle, Rectangle, Vector2, float, Color);
Texture2D LoadTextureFromImage(Image);
void SetTextureFilter(Texture2D, int);
Material LoadMaterialDefault(void);
void UploadMesh(Mesh*, bool);
void UnloadMesh(Mesh);
void DrawMesh(Mesh, Material, Matrix);
void* MemAlloc(unsigned int);
Vector2 GetWorldToScreen(Vector3, Camera);
typedef struct Wave { unsigned int frameCount, sampleRate, sampleSize, channels; void* data; } Wave;
typedef struct Sound { unsigned long long stream[4]; unsigned int frameCount; } Sound;
void InitAudioDevice(void); void CloseAudioDevice(void); bool IsAudioDeviceReady(void); void SetMasterVolume(float);
Sound LoadSoundFromWave(Wave); void PlaySound(Sound); void SetSoundVolume(Sound, float);
void DrawLineEx(Vector2, Vector2, float, Color); void DrawLine(int, int, int, int, Color);
