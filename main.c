#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "raylib.h"
#include "ui.h"
#include "rank.h"
#include "account.h"
#include "game.h"
#define COLOR_TITLE (Color){ 240, 246, 255, 255 }
#define COLOR_HINT  (Color){ 190, 202, 220, 255 }
#define COLOR_OK    (Color){ 120, 220, 160, 255 }
#define COLOR_WARN  (Color){ 250, 200, 120, 255 }
#define COLOR_ERROR (Color){ 255, 130, 130, 255 }
typedef enum {
    PAGE_ENTRY,
    PAGE_ACCOUNT,
    PAGE_MENU,
    PAGE_LEVEL,
    PAGE_CUSTOM_CONFIG,
    PAGE_GAME,
    PAGE_RESULT,
    PAGE_RANK,
    PAGE_INTRODUCE
} Page;
static const Rectangle loginButton = {180, 180, 200, 48};
static const Rectangle registerButton = {420, 180, 200, 48};
static const Rectangle switchButton = {180, 180, 440, 48};
static const Rectangle gameButton = {180, 260, 440, 48};
static const Rectangle rankButton = {180, 340, 440, 48};
static const Rectangle introduceButton = {180, 405, 440, 40};
static const Rectangle introduceBackButton = {300, 530, 200, 44};
static const Rectangle exitButton = {300, 460, 200, 48};
static const Rectangle backButton = {300, 460, 200, 48};
static const Rectangle easyButton = {250, 150, 300, 48};
static const Rectangle mediumButton = {250, 220, 300, 48};
static const Rectangle hardButton = {250, 290, 300, 48};
static const Rectangle customButton = {250, 360, 300, 48};
static const Rectangle customStart = {180, 460, 210, 48};
static const Rectangle customBack = {410, 460, 210, 48};
static const Rectangle resultRankButton = {180, 480, 210, 48};
static const Rectangle resultBackButton = {410, 480, 210, 48};
static void drawCustomConfig(Font font, const CustomGameConfig *cfg);
static void drawCustomResult(Font font, const CustomGameResult *r);
static void drawText(Font font, const char text[], float y, float size, Color color);
static void drawMenu(Font font, int loggedIn);
static void drawIntroduce(Font font);
static void drawLevels(Font font);
int main(void)
{
    Font font = {0};
    Camera2D camera = {0};
    Page page = PAGE_ENTRY;
    User user = {0};
    int accountResult;
    int running = 1;
    int level = 0;
    int seconds = 0;
    int gameResult;
    int saveResult = 0;
    CustomGameConfig config = {4, 4, 5, 1.0};
    CustomGameResult customResult = {0};
    srand((unsigned int)time(NULL));
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(800, 600, "记忆力大考验");
    SetWindowMinSize(800, 600);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    if (!loadUiResources(&font)) { CloseWindow(); return 1; }
    loadGameResources();
    loadBackgrounds();
    while (running && !WindowShouldClose()) {
        camera = getUiCamera();
        SetMouseOffset(-(int)camera.offset.x, -(int)camera.offset.y);
        SetMouseScale(1.0f / camera.zoom, 1.0f / camera.zoom);
        switch (page) {
            case PAGE_ENTRY:
                if (isButtonClicked(loginButton)) {
                    resetAccountPage(0);
                    page = PAGE_ACCOUNT;
                } else if (isButtonClicked(registerButton)) {
                    resetAccountPage(1);
                    page = PAGE_ACCOUNT;
                } else if (isButtonClicked(exitButton)) {
                    running = 0;
                }
                break;
            case PAGE_ACCOUNT:
                accountResult = updateAccountPage(&user);
                if (accountResult == 1) {
                    page = PAGE_MENU;
                } else if (accountResult == 2) {
                    page = PAGE_ENTRY;
                }
                break;
            case PAGE_MENU:
                if (isButtonClicked(switchButton)) {
                    user = (User){0};
                    page = PAGE_ENTRY;
                } else if (isButtonClicked(gameButton)) {
                    page = PAGE_LEVEL;
                } else if (isButtonClicked(rankButton)) {
                    setCurrentPlayer(user.name);
                    resetRankPage();
                    page = PAGE_RANK;
                } else if (isButtonClicked(introduceButton)) {
                    page = PAGE_INTRODUCE;
                } else if (isButtonClicked(exitButton)) {
                    running = 0;
                }
                break;
            case PAGE_LEVEL:
                if (isButtonClicked(easyButton)) {
                    level = 1;
                }
                else if (isButtonClicked(mediumButton)) {
                    level = 2;
                }
                else if (isButtonClicked(hardButton)) {
                    level = 3;
                }
                else if (isButtonClicked(customButton)) {
                    page = PAGE_CUSTOM_CONFIG;
                    break;
                }
                else if (isButtonClicked(backButton)) {
                    page = PAGE_MENU;
                    break;
                }
                else {
                    break;
                }
                startGame(level);
                page = PAGE_GAME;
                break;
            case PAGE_CUSTOM_CONFIG: {
                int i;
                for (i = 0; i < 4; i++) {
                    int delta = isButtonClicked((Rectangle){520, 130.0f+i*55.0f, 48, 40}) -
                                isButtonClicked((Rectangle){300, 130.0f+i*55.0f, 48, 40});
                    if (i == 0 && config.rows+delta >= 2 && config.rows+delta <= 6) config.rows += delta;
                    if (i == 1 && config.cols+delta >= 2 && config.cols+delta <= 6) config.cols += delta;
                    if (i == 2 && config.previewSeconds + delta >= 1 && config.previewSeconds + delta <= 30)
                        config.previewSeconds += delta;
                    if (i == 3) {
                        int tenths = (int)lround(config.k * 10.0) + delta;
                        if (tenths >= 1 && tenths <= 50) config.k = tenths / 10.0;
                    }
                }
                if (isButtonClicked(customBack)) page = PAGE_LEVEL;
                else if (isButtonClicked(customStart) && isEven(&config)) {
                    startCustomGame(&config);
                    page = PAGE_GAME;
                }
                break;
            }
            case PAGE_GAME:
                gameResult = updateGame(&seconds);
                if (gameResult == 1) {
                    saveResult = 0;
                    if (isCustomGame()) {
                        getCustomGameResult(&customResult);
                        saveResult = saveCustomScore(user.name, customResult.finalScore);
                    } else if (!getGameCheats()) {
                        saveResult = saveScore(user.name, level, seconds);
                    }
                    page = PAGE_RESULT;
                } else if (gameResult == 2) {
                    page = PAGE_MENU;
                }
                break;
            case PAGE_RESULT:
                if (isCustomGame()) {
                    if (isButtonClicked(resultBackButton)) page = PAGE_MENU;
                    else if (isButtonClicked(resultRankButton)) {
                        setCurrentPlayer(user.name);
                        resetCustomRankPage();
                        page = PAGE_RANK;
                    }
                    break;
                }
                if (isButtonClicked(backButton)) {
                    page = PAGE_MENU;
                } else if (isButtonClicked(rankButton)) {
                    setCurrentPlayer(user.name);
                    resetRankPage();
                    page = PAGE_RANK;
                }
                break;
            case PAGE_INTRODUCE:
                if (isButtonClicked(introduceBackButton)) page = PAGE_MENU;
                break;
            case PAGE_RANK:
                if (updateRankPage()) {
                    page = PAGE_MENU;
                }
                break;
        }
        BeginDrawing();
        ClearBackground((Color){245, 248, 252, 255});
        if (page == PAGE_GAME)      drawBackground(1);
        else if (page == PAGE_RANK) drawBackground(2);
        else                        drawBackground(0);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                      (Color){ 8, 12, 20, 110 });
        BeginMode2D(camera);
        switch (page) {
            case PAGE_ENTRY:
                drawMenu(font, 0);
                break;
            case PAGE_ACCOUNT:
                drawAccountPage(font);
                break;
            case PAGE_MENU:
                drawMenu(font, 1);
                drawText(font, TextFormat("当前用户：%s", user.name), 125, 20, COLOR_HINT);
                break;
            case PAGE_LEVEL:
                drawLevels(font);
                break;
            case PAGE_CUSTOM_CONFIG:
                drawCustomConfig(font, &config);
                break;
            case PAGE_GAME:
                drawGame(font);
                break;
            case PAGE_RESULT:
                if (isCustomGame()) { drawCustomResult(font, &customResult); break; }
                drawText(font, "恭喜通关", 90, 36, COLOR_TITLE);
                drawText(font, TextFormat("用时：%d 秒", seconds), 180, 28, COLOR_TITLE);
                drawText(font, TextFormat("配对错误：%d 次   作弊：%d 次", getGameMistakes(), getGameCheats()), 215, 20, COLOR_HINT);
                if (getGameCheats() != 0) {
                    drawText(font, "本局使用了亮牌，不计入排行榜", 250, 22, COLOR_WARN);
                } else {
                    if (saveResult == 1) {
                        drawText(font, "成绩已保存", 250, 24, COLOR_OK);
                    } else if (saveResult == 0) {
                        drawText(font, "未刷新成绩", 250, 24, COLOR_HINT);
                    } else {
                        drawText(font, "成绩保存失败", 250, 24, COLOR_ERROR);
                    }
                }
                drawButton(rankButton, "排行榜", font);
                drawButton(backButton, "返回", font);
                break;
            case PAGE_INTRODUCE:
                drawIntroduce(font);
                break;
            case PAGE_RANK:
                drawRankPage(font);
                break;
        }
        EndMode2D();
        EndDrawing();
    }
    unloadGameResources();
    unloadBackgrounds();
    unloadUiResources(font);
    CloseWindow();
    return 0;
}
static void drawText(Font font, const char text[], float y, float size, Color color)
{
    Vector2 width = MeasureTextEx(font, text, size, 1);
    DrawTextEx(font, text, (Vector2){(800 - width.x) / 2, y}, size, 1, color);
}
static void drawMenu(Font font, int loggedIn)
{
    drawText(font, "记忆力大考验", 70, 40, COLOR_TITLE);
    if (loggedIn) {
        drawButton(switchButton, "切换账号", font);
        drawButton(gameButton, "开始游戏", font);
        drawButton(rankButton, "排行榜", font);
        drawButton(introduceButton, "玩法说明", font);
    } else {
        drawButton(loginButton, "登录", font);
        drawButton(registerButton, "注册", font);
        drawText(font, "请先登录", 285, 24, COLOR_HINT);
    }
    drawButton(exitButton, "退出", font);
}
static void drawLevels(Font font)
{
    drawText(font, "选择难度", 80, 36, COLOR_TITLE);
    drawButton(easyButton, "初级 4x4", font);
    drawButton(mediumButton, "中级 4x5", font);
    drawButton(hardButton, "高级 6x6", font);
    drawButton(customButton, "自定义积分挑战", font);
    drawButton(backButton, "返回", font);
}
static void drawCustomConfig(Font font, const CustomGameConfig *cfg)
{
    int i, groups = cfg->rows * cfg->cols / 2;
    const char *labels[] = {"棋盘行数", "棋盘列数", "亮牌时间", "耗时参数 k"};
    drawText(font, "自定义积分挑战", 70, 34, COLOR_TITLE);
    for (i = 0; i < 4; i++) {
        DrawTextEx(font, labels[i], (Vector2){130, 138.0f+i*55.0f}, 22, 1, COLOR_TITLE);
        drawButton((Rectangle){300, 130.0f+i*55.0f, 48, 40}, "-", font);
        drawButton((Rectangle){520, 130.0f+i*55.0f, 48, 40}, "+", font);
        const char *value;
        if (i == 3) value = TextFormat("%.1f", cfg->k);
        else if (i == 2) value = TextFormat("%d 秒", cfg->previewSeconds);
        else value = TextFormat("%d", i == 0 ? cfg->rows : cfg->cols);
        drawText(font, value, 138.0f+i*55.0f, 24, COLOR_TITLE);
    }
    drawText(font, TextFormat("牌数 %d  配对 %d 组", cfg->rows * cfg->cols, groups), 360, 20, COLOR_TITLE);
    if (isEven(cfg)) {
        CustomGameResult preview;
        calculateCustomResult(cfg, 0, 0, 0, 0.0, &preview);
        drawText(font, TextFormat("总分倍率：棋盘 %.2f 倍   开局亮牌 %.2f 倍", preview.B_plus, 1.0 / preview.K), 395, 20, COLOR_TITLE);
        drawText(font, "k 越大，最快通关加分越多，基准前后得分变化越快", 425, 18, COLOR_HINT);
        drawButton(customStart, "开始挑战", font);
    } else {
        drawText(font, "牌数必须为偶数", 425, 20, COLOR_ERROR);
    }
    drawButton(customBack, "返回", font);
    drawText(font, "亮牌时间到后自动盖牌并开始计时", 540, 18, COLOR_HINT);
}
static void drawCustomResult(Font font, const CustomGameResult *r)
{
    drawText(font, "挑战完成", 55, 34, COLOR_TITLE);
    drawText(font, TextFormat("正确 %d 组  错误 %d 次  基础积分 %.2f", r->R, r->W, r->S), 120, 22, COLOR_TITLE);
    drawText(font, TextFormat("棋盘倍率 %.3f", r->B_plus), 165, 22, COLOR_HINT);
    drawText(font, TextFormat("开局展示 %.2f 秒 / 基准 %.2f 秒", r->T, r->T0), 210, 22, COLOR_TITLE);
    drawText(font, TextFormat("亮牌时间倍率 %.3f", r->K), 250, 22, COLOR_HINT);
    drawText(font, TextFormat("用时 %.2f 秒 / 基准 %.2f 秒", r->t, r->Tref), 290, 22, COLOR_TITLE);
    drawText(font, TextFormat("主动亮牌 %d 次  剩余 %.2f%%", r->H, 100 * r->F_H), 330, 22, COLOR_HINT);
    drawText(font, TextFormat("耗时倍率 %.3f", 1.0 - r->B_minus), 370, 22, COLOR_HINT);
    drawText(font, TextFormat("最终积分 %d 分", r->finalScore), 415, 30, COLOR_OK);
    drawButton(resultRankButton, "查看积分榜", font);
    drawButton(resultBackButton, "返回菜单", font);
}
static void drawIntroduce(Font font)
{
    drawText(font, "玩法说明", 40, 34, COLOR_TITLE);
    drawText(font, "基本玩法", 100, 24, COLOR_OK);
    drawText(font, "先记住开局展示的图案，再点击卡牌寻找相同图案。", 140, 20, COLOR_TITLE);
    drawText(font, "初级 4x4、中级 4x5 两张配对；高级 6x6 三张配对。", 175, 20, COLOR_TITLE);
    drawText(font, "错配后保持亮面并锁定 1 秒，到期同时盖回。", 210, 20, COLOR_TITLE);
    drawText(font, "亮牌可查看全部图案 4 秒；游戏中的返回按钮退出本局。", 245, 20, COLOR_TITLE);
    drawText(font, "普通模式按用时排名，使用亮牌的局不入榜。", 280, 20, COLOR_HINT);
    drawText(font, "自定义挑战", 330, 24, COLOR_OK);
    drawText(font, "行列可选 2 到 6，总牌数须为偶数；开局按设置时间亮牌。", 370, 20, COLOR_TITLE);
    drawText(font, "固定两张配对，错配立即盖回并锁定 1 秒。", 405, 20, COLOR_TITLE);
    drawText(font, "通关按棋盘及展示时间调整积分；每次主动亮牌再乘 0.97。", 440, 20, COLOR_TITLE);
    drawText(font, "自定义模式只显示本局积分。", 475, 19, COLOR_HINT);
    drawButton(introduceBackButton, "返回", font);
}
