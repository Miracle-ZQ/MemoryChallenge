#include <math.h>
#include "raylib.h"

static void polygon(Vector2 center, Vector2 points[], int count, Color color)
{
    int i;
    for (i = 0; i < count; i++) DrawTriangle(center, points[(i + 1) % count], points[i], color);
}

static void drawFace(int id)
{
    int i;
    Vector2 points[10];
    Color colors[12] = {(Color){0, 150, 145, 255}, BLUE, ORANGE, PURPLE, GOLD, DARKGREEN,
                        MAROON, SKYBLUE, DARKPURPLE, PINK, (Color){242, 178, 0, 255}, LIME};
    Color color = colors[id % 12];
    Vector2 center = {256, 256};
    switch (id) {
        case 0:
            DrawRectangle(126, 94, 260, 16, color);
            DrawTriangle((Vector2){130, 110}, (Vector2){256, 256}, (Vector2){382, 110}, color);
            DrawTriangle((Vector2){256, 256}, (Vector2){130, 402}, (Vector2){382, 402}, color);
            DrawRectangle(126, 402, 260, 16, color);
            break;
        case 1: DrawPoly(center, 3, 166, -90, color); break;
        case 2: DrawRectangle(124, 124, 264, 264, color); break;
        case 3: DrawPoly(center, 4, 170, 0, color); break;
        case 4:
            for (i = 0; i < 10; i++) {
                float angle = (-90.0f + i * 36.0f) * DEG2RAD;
                float radius = i % 2 == 0 ? 170 : 74;
                points[i] = (Vector2){256 + cosf(angle) * radius, 256 + sinf(angle) * radius};
            }
            polygon(center, points, 10, color);
            break;
        case 5: DrawPoly(center, 6, 155, 0, color); break;
        case 6:
            DrawRectangle(210, 105, 92, 302, color);
            DrawRectangle(105, 210, 302, 92, color);
            break;
        case 7:
            DrawCircleV(center, 150, color);
            DrawCircleV(center, 89, WHITE);
            break;
        case 8:
            DrawCircleV(center, 150, color);
            DrawCircle(320, 205, 133, WHITE);
            break;
        case 9:
            DrawCircle(185, 190, 86, color);
            DrawCircle(327, 190, 86, color);
            DrawTriangle((Vector2){99, 205}, (Vector2){256, 405}, (Vector2){413, 205}, color);
            break;
        case 10:
            points[0]=(Vector2){275,80};points[1]=(Vector2){135,280};
            points[2]=(Vector2){235,280};points[3]=(Vector2){215,435};
            points[4]=(Vector2){380,215};points[5]=(Vector2){270,215};

            DrawTriangle(points[0], points[1], points[2], color);
            DrawTriangle(points[0], points[2], points[5], color);
            DrawTriangle(points[2], points[3], points[4], color);
            DrawTriangle(points[2], points[4], points[5], color);
            break;
        case 11:
            for (i = 0; i < 6; i++) {
                float angle = i * 60 * DEG2RAD;
                DrawCircle(256 + (int)(92 * cosf(angle)), 256 + (int)(92 * sinf(angle)), 66, color);
            }
            DrawCircleV(center, 54, GOLD);
            break;
        case 12:
            DrawCircleV(center, 95, color);
            for (i = 0; i < 12; i++) {
                float a = i * 30 * DEG2RAD;
                DrawLineEx((Vector2){256+120*cosf(a),256+120*sinf(a)},
                           (Vector2){256+170*cosf(a),256+170*sinf(a)},14,color);
            }
            break;
        case 13:
            DrawCircle(190,240,75,color); DrawCircle(270,200,95,color);
            DrawCircle(345,255,65,color); DrawRectangle(155,240,220,90,color);
            break;
        case 14:
            DrawCircleSector((Vector2){256,260},150,180,360,36,color);
            DrawLineEx((Vector2){256,260},(Vector2){256,365},14,color);
            DrawRing((Vector2){228,365},22,36,0,180,24,color);
            break;
        case 15:
            DrawTriangle((Vector2){90,390},(Vector2){430,390},(Vector2){260,105},color);
            DrawTriangle((Vector2){212,185},(Vector2){308,185},(Vector2){260,105},WHITE);
            break;
        case 16:
            DrawRectangle(238,285,36,130,color);
            DrawTriangle((Vector2){100,310},(Vector2){412,310},(Vector2){256,80},color);
            break;
        case 17:
            DrawTriangle((Vector2){80,235},(Vector2){432,235},(Vector2){256,90},color);
            DrawRectangle(125,235,262,170,color); DrawRectangle(225,295,62,110,WHITE);
            break;
        case 18:
            DrawRing((Vector2){180,200},45,73,0,360,40,color);
            DrawLineEx((Vector2){225,245},(Vector2){365,385},28,color);
            DrawLineEx((Vector2){320,340},(Vector2){355,305},28,color);
            break;
        case 19:
            DrawRing((Vector2){256,205},65,90,180,360,36,color);
            DrawRectangle(140,205,232,190,color); DrawCircle(256,280,23,WHITE);
            DrawRectangle(246,280,20,60,WHITE);
            break;
        case 20:
            DrawRectangle(145,90,18,330,color);
            DrawTriangle((Vector2){163,100},(Vector2){163,280},(Vector2){390,190},color);
            break;
        case 21:
            DrawRectangle(90,215,205,82,color);
            DrawTriangle((Vector2){270,120},(Vector2){270,392},(Vector2){430,256},color);
            break;
        case 22:
            DrawCircle(185,350,55,color); DrawRectangle(225,110,18,240,color);
            DrawRectanglePro((Rectangle){235,110,130,38},(Vector2){0,0},20,color);
            break;
        case 23:
            DrawTriangle((Vector2){100,120},(Vector2){100,392},center,color);
            DrawTriangle(center,(Vector2){412,392},(Vector2){412,120},color);
            DrawCircleV(center,32,color);
            break;
        case 24:
            DrawEllipse(256,256,180,95,color); DrawEllipse(256,256,135,63,WHITE);
            DrawCircleV(center,48,color);
            break;
        case 25:
            points[0]=(Vector2){105,150};points[1]=(Vector2){200,240};
            points[2]=(Vector2){256,100};points[3]=(Vector2){312,240};
            points[4]=(Vector2){407,150};points[5]=(Vector2){365,380};points[6]=(Vector2){147,380};
            polygon((Vector2){256,290},points,7,color);
            break;
        case 26:
            DrawTriangle((Vector2){90,300},(Vector2){256,420},(Vector2){422,300},color);
            DrawRectangle(245,90,16,195,color);
            DrawTriangle((Vector2){230,100},(Vector2){100,275},(Vector2){230,275},color);
            break;
        case 27:
            DrawEllipse(225,256,120,80,color);
            DrawTriangle((Vector2){320,256},(Vector2){420,350},(Vector2){420,162},color);
            DrawCircle(160,235,12,WHITE);
            break;
        case 28:
            DrawRing((Vector2){355,245},38,60,0,360,32,color);
            DrawRectangle(125,160,220,180,color); DrawRectangle(100,365,300,16,color);
            break;
        case 29:
            DrawRectangle(120,210,272,190,color); DrawRectangle(100,175,312,45,color);
            DrawRectangle(240,175,32,225,WHITE);
            DrawRing((Vector2){210,140},24,38,0,360,24,color);
            DrawRing((Vector2){302,140},24,38,0,360,24,color);
            break;
        case 30:
            DrawRectangleRounded((Rectangle){110,110,292,292},0.15f,12,color);
            DrawCircle(180,180,22,WHITE);DrawCircle(332,180,22,WHITE);
            DrawCircle(256,256,22,WHITE);DrawCircle(180,332,22,WHITE);DrawCircle(332,332,22,WHITE);
            break;
        case 31:
            DrawRing(center,135,155,0,360,48,color);
            DrawLineEx(center,(Vector2){256,150},18,color);
            DrawLineEx(center,(Vector2){335,300},18,color);
            break;
        case 32:
            DrawTriangle((Vector2){80,220},(Vector2){240,280},(Vector2){425,90},color);
            DrawTriangle((Vector2){240,280},(Vector2){330,420},(Vector2){425,90},color);
            break;
        case 33:
            DrawRectangleRounded((Rectangle){230,90,52,320},0.8f,12,color);
            DrawLineEx((Vector2){150,180},(Vector2){150,280},38,color);
            DrawLineEx((Vector2){150,280},(Vector2){250,280},38,color);
            DrawLineEx((Vector2){360,130},(Vector2){360,225},38,color);
            DrawLineEx((Vector2){360,225},(Vector2){260,225},38,color);
            break;
        case 34:
            DrawRectangle(215,245,82,155,color);
            DrawCircleSector((Vector2){256,260},160,180,360,36,color);
            DrawCircle(190,210,24,WHITE); DrawCircle(290,160,22,WHITE);DrawCircle(340,230,18,WHITE);
            break;
        case 35:
            DrawRectangle(155,90,24,330,color);DrawRectangle(333,90,24,330,color);
            for(i=0;i<5;i++) DrawRectangle(179,120+i*60,154,18,color);
            break;
    }
}

int main(void)
{
    int id, ok = 1;
    Image image;
    Camera2D camera = {0};
    RenderTexture2D canvas;
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(64, 64, "Original card asset generator");
    canvas = LoadRenderTexture(1024, 1024);
    camera.zoom = 2;
    for (id = 0; id <= 36; id++) {
        BeginTextureMode(canvas);
        ClearBackground(WHITE);
        BeginMode2D(camera);
        if (id < 36) {
            drawFace(id);
        } else {
            DrawRectangle(0, 0, 512, 512, (Color){42, 75, 125, 255});
            DrawRectangleLinesEx((Rectangle){22, 22, 468, 468}, 5, (Color){155, 194, 225, 255});
            DrawPoly((Vector2){256, 256}, 4, 110, 0, (Color){155, 194, 225, 255});
            DrawPoly((Vector2){256, 256}, 4, 75, 0, (Color){42, 75, 125, 255});
        }
        EndMode2D();
        EndTextureMode();
        image = LoadImageFromTexture(canvas.texture);
        ImageFlipVertical(&image);
        ImageResize(&image, 512, 512);
        if (!ExportImage(image, id == 36 ? "assets/cards/back.png" : TextFormat("assets/cards/%02d.png", id))) ok = 0;
        UnloadImage(image);
    }
    UnloadRenderTexture(canvas);
    CloseWindow();
    return ok ? 0 : 1;
}
