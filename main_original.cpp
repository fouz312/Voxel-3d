
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <string>

enum class Block : unsigned char {
    Air, Grass, Dirt, Stone, Bedrock, Wood, Leaves,
    CoalOre, IronOre, GoldOre, DiamondOre, Planks, Glass, Brick,
    Netherrack, SoulSand, Glowstone, QuartzOre, Obsidian,
    EndStone, Purpur, EndPortalFrame
};

struct BlockInfo {
    Color color;
    bool solid;
    float hardness;
    const char* name;
};

static BlockInfo Info(Block b) {
    switch (b) {
        case Block::Grass: return {{86, 154, 57,255},true,1.2f,"Grass Block"};
        case Block::Dirt: return {{121,85,58,255},true,1.0f,"Dirt"};
        case Block::Stone: return {{125,125,125,255},true,3.0f,"Stone"};
        case Block::Bedrock: return {{45,45,45,255},true,9999.f,"Bedrock"};
        case Block::Wood: return {{126,83,45,255},true,2.0f,"Oak Wood"};
        case Block::Leaves: return {{54,125,45,255},true,.4f,"Oak Leaves"};
        case Block::CoalOre: return {{105,105,105,255},true,3.5f,"Coal Ore"};
        case Block::IronOre: return {{105,105,105,255},true,4.5f,"Iron Ore"};
        case Block::GoldOre: return {{105,105,105,255},true,5.f,"Gold Ore"};
        case Block::DiamondOre: return {{105,105,105,255},true,6.f,"Diamond Ore"};
        case Block::Planks: return {{190,145,90,255},true,1.8f,"Oak Planks"};
        case Block::Glass: return {{180,225,235,150},true,.6f,"Glass"};
        case Block::Brick: return {{170,85,70,255},true,3.f,"Bricks"};
        case Block::Netherrack: return {{110,35,35,255},true,1.f,"Netherrack"};
        case Block::SoulSand: return {{80,58,46,255},true,1.5f,"Soul Sand"};
        case Block::Glowstone: return {{245,185,55,255},true,1.f,"Glowstone"};
        case Block::QuartzOre: return {{110,35,35,255},true,2.5f,"Nether Quartz Ore"};
        case Block::Obsidian: return {{35,18,55,255},true,12.f,"Obsidian"};
        case Block::EndStone: return {{225,225,175,255},true,3.5f,"End Stone"};
        case Block::Purpur: return {{170,115,170,255},true,2.5f,"Purpur"};
        case Block::EndPortalFrame: return {{20,90,80,255},true,10.f,"End Portal Frame"};
        default: return {{0,0,0,0},false,0,"Air"};
    }
}

struct VoxelWorld {
    int w = 64, h = 48, d = 64;
    std::vector<Block> blocks;
    VoxelWorld() : blocks(w*h*d, Block::Air) {}

    int index(int x,int y,int z) const { return x + w*(z + d*y); }
    bool inside(int x,int y,int z) const {
        return x>=0 && x<w && y>=0 && y<h && z>=0 && z<d;
    }
    Block get(int x,int y,int z) const {
        if(!inside(x,y,z)) return Block::Bedrock;
        return blocks[index(x,y,z)];
    }
    void set(int x,int y,int z,Block b) {
        if(inside(x,y,z)) blocks[index(x,y,z)] = b;
    }
    bool solid(int x,int y,int z) const { return Info(get(x,y,z)).solid; }
};

static float noise2(float x,float z) {
    return (sinf(x*0.13f)+cosf(z*0.11f)+sinf((x+z)*0.055f)*1.7f)/3.7f;
}

static void PlantTree(VoxelWorld& v,int x,int y,int z) {
    for(int i=0;i<5 && y+i<v.h;i++) v.set(x,y+i,z,Block::Wood);
    for(int dx=-2;dx<=2;dx++)
        for(int dz=-2;dz<=2;dz++)
            for(int dy=2;dy<=4;dy++)
                if(std::abs(dx)+std::abs(dz) < 4 && v.get(x+dx,y+dy,z+dz)==Block::Air)
                    v.set(x+dx,y+dy,z+dz,Block::Leaves);
}

static void GenerateOverworld(VoxelWorld& v) {
    for(int x=0;x<v.w;x++) for(int z=0;z<v.d;z++) {
        int surface = 20 + (int)(noise2((float)x,(float)z)*7.f);
        surface = std::clamp(surface,7,v.h-8);
        for(int y=0;y<v.h;y++) {
            if(y==0) v.set(x,y,z,Block::Bedrock);
            else if(y>surface+7) {
                float r = fmodf(fabsf(sinf(x*12.91f+z*78.31f)),1.f);
                v.set(x,y,z, r<.025f?Block::DiamondOre:r<.06f?Block::GoldOre:
                    r<.12f?Block::IronOre:r<.19f?Block::CoalOre:Block::Stone);
            } else if(y>surface) v.set(x,y,z,Block::Dirt);
            else if(y==surface) v.set(x,y,z,Block::Grass);
        }
        if(((x*37+z*17)%23)==0 && surface>8) PlantTree(v,x,surface+1,z);
    }
}

static void GenerateNether(VoxelWorld& v) {
    for(int x=0;x<v.w;x++) for(int z=0;z<v.d;z++)
        for(int y=0;y<v.h;y++) {
            if(y==0 || y==v.h-1) v.set(x,y,z,Block::Bedrock);
            else {
                float n = sinf(x*.18f)+cosf(z*.14f)+sinf(y*.27f);
                if(n > -0.9f) {
                    float r=fmodf(fabsf(sinf(x*4.1f+y*2.7f+z*6.3f)),1.f);
                    v.set(x,y,z,r<.05f?Block::QuartzOre:r<.09f?Block::Glowstone:Block::Netherrack);
                }
            }
        }
}

static void GenerateEnd(VoxelWorld& v) {
    for(int x=0;x<v.w;x++) for(int z=0;z<v.d;z++) {
        float dx=x-v.w/2.f, dz=z-v.d/2.f;
        float dist=sqrtf(dx*dx+dz*dz);
        if(dist < 21.f + 3.f*sinf(x*.3f)*cosf(z*.25f))
            for(int y=12;y<25;y++) v.set(x,y,z,Block::EndStone);
    }
    for(int px : {16,32,48}) for(int z=16;z<49;z+=16)
        for(int y=12;y<30;y++) v.set(px,y,z,Block::Obsidian);
}

struct Player {
    Vector3 pos{32,27,32};
    Vector3 vel{0,0,0};
    float yaw=0, pitch=0;
    bool grounded=false;
};

static bool Collides(const VoxelWorld& v, Vector3 p) {
    const float r=.28f, height=1.75f;
    for(int x=(int)floorf(p.x-r);x<=(int)floorf(p.x+r);x++)
        for(int z=(int)floorf(p.z-r);z<=(int)floorf(p.z+r);z++)
            for(int y=(int)floorf(p.y);y<=(int)floorf(p.y+height);y++)
                if(v.solid(x,y,z)) return true;
    return false;
}

static void MovePlayer(const VoxelWorld& v, Player& p, Vector3 wish, float dt) {
    p.vel.x = wish.x * 5.0f;
    p.vel.z = wish.z * 5.0f;
    p.vel.y -= 22.0f*dt;

    Vector3 q=p.pos;
    q.x += p.vel.x*dt;
    if(!Collides(v,q)) p.pos.x=q.x; else p.vel.x=0;
    q=p.pos; q.z += p.vel.z*dt;
    if(!Collides(v,q)) p.pos.z=q.z; else p.vel.z=0;
    q=p.pos; q.y += p.vel.y*dt;
    if(!Collides(v,q)) {p.pos.y=q.y;p.grounded=false;}
    else {
        if(p.vel.y<0) p.grounded=true;
        p.vel.y=0;
    }
    p.pos.x=Clamp(p.pos.x,.5f,v.w-.5f);
    p.pos.z=Clamp(p.pos.z,.5f,v.d-.5f);
}

static bool RaycastVoxel(const VoxelWorld& v, Ray ray, Vector3& hit, Vector3& normal) {
    Vector3 p=ray.position;
    for(int i=0;i<160;i++) {
        int x=(int)floorf(p.x), y=(int)floorf(p.y), z=(int)floorf(p.z);
        if(v.inside(x,y,z) && v.solid(x,y,z)) {
            hit={(float)x,(float)y,(float)z};
            Vector3 prev=p-ray.direction*.035f;
            int px=(int)floorf(prev.x), py=(int)floorf(prev.y), pz=(int)floorf(prev.z);
            normal={(float)(x-px),(float)(y-py),(float)(z-pz)};
            return true;
        }
        p += ray.direction*.035f;
        if(Vector3Distance(ray.position,p)>7.f) break;
    }
    return false;
}

static void DrawVoxel(const VoxelWorld& v,int x,int y,int z) {
    Block b=v.get(x,y,z);
    if(b==Block::Air) return;
    Color c=Info(b).color;
    Vector3 pos={(float)x+.5f,(float)y+.5f,(float)z+.5f};
    DrawCube(pos,1,1,1,c);
    // Original stylized face accents; deliberately not Minecraft textures.
    if(b==Block::Grass) {
        DrawCube({pos.x,pos.y+.46f,pos.z},1.01f,.08f,1.01f,Color{92,170,55,255});
        DrawCube({pos.x,pos.y-.46f,pos.z},1.01f,.08f,1.01f,Color{110,75,48,255});
    }
}

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(1280,720,"Voxel C++ 3D - Original Block World");
    SetTargetFPS(60);
    DisableCursor();

    VoxelWorld worlds[3];
    GenerateOverworld(worlds[0]);
    GenerateNether(worlds[1]);
    GenerateEnd(worlds[2]);

    int dimension=0;
    Player player;
    player.pos={32,35,32};

    Camera3D cam{};
    cam.position=player.pos;
    cam.target=Vector3Add(player.pos,{0,0,1});
    cam.up={0,1,0};
    cam.fovy=75;
    cam.projection=CAMERA_PERSPECTIVE;

    Block hotbar[9]={Block::Dirt,Block::Stone,Block::Wood,Block::Planks,
                     Block::Glass,Block::Obsidian,Block::IronOre,Block::DiamondOre,Block::Brick};
    int selected=0;
    int counts[9]={64,64,32,32,16,8,0,0,32};
    bool placeMode=false;
    Vector3 selectedBlock{-1,-1,-1};
    float mineProgress=0;

    while(!WindowShouldClose()) {
        float dt=std::min(GetFrameTime(),.05f);

        if(IsKeyPressed(KEY_ONE)) selected=0;
        if(IsKeyPressed(KEY_TWO)) selected=1;
        if(IsKeyPressed(KEY_THREE)) selected=2;
        if(IsKeyPressed(KEY_FOUR)) selected=3;
        if(IsKeyPressed(KEY_FIVE)) selected=4;
        if(IsKeyPressed(KEY_SIX)) selected=5;
        if(IsKeyPressed(KEY_SEVEN)) selected=6;
        if(IsKeyPressed(KEY_EIGHT)) selected=7;
        if(IsKeyPressed(KEY_NINE)) selected=8;
        if(IsKeyPressed(KEY_TAB)) placeMode=!placeMode;
        if(IsKeyPressed(KEY_P)) { dimension=(dimension+1)%3; player.pos={32,35,32}; }
        if(IsKeyPressed(KEY_SPACE) && player.grounded) player.vel.y=8.2f;

        // Mobile touch controls:
        // left bottom = virtual movement pad, right buttons = jump / mine-place.
        Vector2 touchMove={0,0};
        bool touchJump=false, touchMine=false, touchPlace=false;
        int touchCount=GetTouchPointCount();
        for(int ti=0; ti<touchCount; ++ti) {
            Vector2 tp=GetTouchPosition(ti);
            if(tp.x < GetScreenWidth()*0.34f && tp.y > GetScreenHeight()*0.62f) {
                Vector2 center={95,(float)GetScreenHeight()-105};
                Vector2 d=Vector2Subtract(tp,center);
                float len=Vector2Length(d);
                if(len>8) touchMove=Vector2Scale(Vector2Normalize(d),Clamp(len/62.f,0.f,1.f));
            } else if(tp.x > GetScreenWidth()-150 && tp.y > GetScreenHeight()-185) {
                touchJump=true;
            } else if(tp.x > GetScreenWidth()-250 && tp.y > GetScreenHeight()-150) {
                touchMine=true;
            }
        }

        // A short tap on the bottom-right action area toggles mine/place.
        if(IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) placeMode=!placeMode;

        Vector2 mouse=GetMouseDelta();
        // On touch screens, dragging in the upper/right play area rotates the camera.
        if(touchCount>0) {
            Vector2 tp=GetTouchPosition(0);
            if(tp.y < GetScreenHeight()*0.58f && tp.x > GetScreenWidth()*0.30f) {
                Vector2 td=GetTouchDelta(0);
                player.yaw += td.x*.004f;
                player.pitch=Clamp(player.pitch-td.y*.004f,-1.45f,1.45f);
            }
        }
        player.yaw += mouse.x*.0025f;
        player.pitch=Clamp(player.pitch-mouse.y*.0025f,-1.45f,1.45f);

        Vector3 forward={sinf(player.yaw)*cosf(player.pitch),sinf(player.pitch),cosf(player.yaw)*cosf(player.pitch)};
        Vector3 flatForward={sinf(player.yaw),0,cosf(player.yaw)};
        Vector3 right={cosf(player.yaw),0,-sinf(player.yaw)};
        Vector3 wish={0,0,0};
        if(IsKeyDown(KEY_W)) wish=Vector3Add(wish,flatForward);
        if(IsKeyDown(KEY_S)) wish=Vector3Subtract(wish,flatForward);
        if(IsKeyDown(KEY_D)) wish=Vector3Add(wish,right);
        if(IsKeyDown(KEY_A)) wish=Vector3Subtract(wish,right);
        if(Vector2Length(touchMove)>0) {
            wish=Vector3Add(wish,Vector3Scale(flatForward,touchMove.y));
            wish=Vector3Add(wish,Vector3Scale(right,touchMove.x));
        }
        if(Vector3Length(wish)>0) wish=Vector3Normalize(wish);
        if((touchJump || IsKeyDown(KEY_SPACE)) && player.grounded) player.vel.y=8.2f;
        MovePlayer(worlds[dimension],player,wish,dt);

        cam.position=Vector3Add(player.pos,{0,1.55f,0});
        cam.target=Vector3Add(cam.position,forward);

        Ray ray={cam.position,forward};
        Vector3 hit,normal;
        bool hasHit=RaycastVoxel(worlds[dimension],ray,hit,normal);

        bool mineHeld = IsMouseButtonDown(MOUSE_BUTTON_LEFT) || touchMine;
        if(mineHeld && hasHit && !placeMode) {
            if(selectedBlock.x!=hit.x||selectedBlock.y!=hit.y||selectedBlock.z!=hit.z) {
                selectedBlock=hit; mineProgress=0;
            }
            Block b=worlds[dimension].get((int)hit.x,(int)hit.y,(int)hit.z);
            if(b!=Block::Bedrock) {
                mineProgress += dt*(1.7f/Info(b).hardness);
                if(mineProgress>=1) {
                    worlds[dimension].set((int)hit.x,(int)hit.y,(int)hit.z,Block::Air);
                    mineProgress=0; selectedBlock={-1,-1,-1};
                }
            }
        } else if((IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || (touchMine && placeMode)) && hasHit && counts[selected]>0) {
            int x=(int)(hit.x+normal.x), y=(int)(hit.y+normal.y), z=(int)(hit.z+normal.z);
            if(worlds[dimension].inside(x,y,z) && !worlds[dimension].solid(x,y,z)) {
                worlds[dimension].set(x,y,z,hotbar[selected]);
                counts[selected]--;
            }
        } else if(!mineHeld) {
            mineProgress=0;
        }

        // Mobile-friendly on-screen controls: left virtual pad + right action buttons.
        // Desktop mouse/keyboard remains fully usable.
        BeginDrawing();
        ClearBackground(dimension==0?Color{110,190,235,255}:dimension==1?Color{75,20,20,255}:Color{25,18,42,255});
        BeginMode3D(cam);

        int minX=std::max(0,(int)player.pos.x-14), maxX=std::min(worlds[dimension].w-1,(int)player.pos.x+14);
        int minZ=std::max(0,(int)player.pos.z-14), maxZ=std::min(worlds[dimension].d-1,(int)player.pos.z+14);
        for(int x=minX;x<=maxX;x++)
            for(int z=minZ;z<=maxZ;z++)
                for(int y=0;y<worlds[dimension].h;y++)
                    if(worlds[dimension].get(x,y,z)!=Block::Air) DrawVoxel(worlds[dimension],x,y,z);

        if(hasHit) {
            DrawCubeWires({hit.x+.5f,hit.y+.5f,hit.z+.5f},1.04f,1.04f,1.04f,WHITE);
            if(mineProgress>0) {
                DrawCubeWires({hit.x+.5f,hit.y+.5f,hit.z+.5f},1.08f,1.08f,1.08f,RED);
            }
        }
        EndMode3D();

        DrawRectangle(12,12,370,72,Fade(BLACK,.55f));
        DrawText(TextFormat("3D VOXEL WORLD  |  %s",dimension==0?"OVERWORLD":dimension==1?"NETHER":"THE END"),24,24,20,RAYWHITE);
        DrawText("WASD move  |  Mouse look  |  Space jump  |  LMB mine  |  RMB place",24,50,12,Color{220,220,220,255});
        DrawText(TextFormat("Selected: %s x%d  |  [TAB] %s  |  [P] dimension",
                 Info(hotbar[selected]).name,counts[selected],placeMode?"PLACE":"MINE"),24,68,12,GOLD);

        // Mobile control overlay.
        int sw=GetScreenWidth(), sh=GetScreenHeight();
        DrawCircle(95,sh-105,62,Fade(BLACK,.32f));
        DrawCircle(95,sh-105,26,Fade(WHITE,.28f));
        DrawText("MOVE",58,sh-180,14,Fade(WHITE,.8f));
        DrawCircle(sw-90,sh-110,52,Fade(BLACK,.35f));
        DrawText("JUMP",sw-116,sh-126,14,WHITE);
        DrawCircle(sw-200,sh-90,40,Fade(BLACK,.35f));
        DrawText(placeMode?"PLACE":"MINE",sw-235,sh-96,13,WHITE);

        DrawRectangle(sw/2-210,sh-62,420,46,Fade(BLACK,.55f));
        for(int i=0;i<9;i++) {
            int x=sw/2-202+i*45;
            DrawRectangleLines(x,sh-55,38,38,i==selected?GOLD:GRAY);
            DrawRectangle(x+7,sh-48,24,24,Info(hotbar[i]).color);
            DrawText(TextFormat("%d",i+1),x+3,sh-53,10,WHITE);
        }
        EndDrawing();
    }

    EnableCursor();
    CloseWindow();
    return 0;
}
