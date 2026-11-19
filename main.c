#include <stdio.h>
#include <stdlib.h>
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
static void drawCustomResult(Font font, const CustomGameResult *r, int saveResult);


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

    CustomGameConfig config = {4, 4, 5};
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
                for (i = 0; i < 3; i++) {

                    int delta = isButtonClicked((Rectangle){520, 160.0f+i*65.0f, 48, 40}) -
                                isButtonClicked((Rectangle){300, 160.0f+i*65.0f, 48, 40});
                    if (i == 0 && config.rows+delta >= 2 && config.rows+delta <= 6) config.rows += delta;
                    if (i == 1 && config.cols+delta >= 2 && config.cols+delta <= 6) config.cols += delta;
                    if (i == 2 && config.previewSeconds+delta >= 1 && config.previewSeconds+delta <= 30) config.previewSeconds += delta;
                }
                if (isButtonClicked(customBack)) page = PAGE_LEVEL;
                else if (isButtonClicked(customStart) && validateCustomConfig(&config)) {
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
                if (isCustomGame()) { drawCustomResult(font, &customResult, saveResult); break; }
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
    drawButton(customButton, "自定义挑战", font);
    drawButton(backButton, "返回", font);
}







static void drawCustomConfig(Font font, const CustomGameConfig *cfg)
{
    int i, groups = cfg->rows * cfg->cols / 2;
    const char *labels[] = {"棋盘行数", "棋盘列数", "开局展示秒数"};
    double b = 1+.04*(groups-8), p = 1+.05*(5-cfg->previewSeconds);
    if (p < .5) p = .5;
    drawText(font, "自定义积分挑战", 70, 34, COLOR_TITLE);
    for (i = 0; i < 3; i++) {
        DrawTextEx(font, labels[i], (Vector2){130, 168.0f+i*65.0f}, 22, 1, COLOR_TITLE);
        drawButton((Rectangle){300, 160.0f+i*65.0f, 48, 40}, "-", font);
        drawButton((Rectangle){520, 160.0f+i*65.0f, 48, 40}, "+", font);
        drawText(font, TextFormat("%d", i == 0 ? cfg->rows : i == 1 ? cfg->cols : (int)cfg->previewSeconds), 168.0f+i*65.0f, 24, COLOR_TITLE);
    }
    drawText(font, "错配亮牌锁定 1 秒，同时盖回；仅两两配对", 360, 20, COLOR_HINT);
    drawText(font, TextFormat("牌数 %d  配对 %d 组  基准 %d 秒", cfg->rows*cfg->cols, groups, 5*groups), 395, 20, COLOR_TITLE);
    drawText(font, TextFormat("棋盘倍率 %.2f  开局倍率 %.2f", b, p), 425, 20, COLOR_HINT);
    if (validateCustomConfig(cfg)) drawButton(customStart, "开始挑战", font);
    else drawText(font, "牌数必须为偶数", 520, 20, COLOR_ERROR);
    drawButton(customBack, "返回", font);
    drawText(font, "主动亮牌每次剩余积分乘 0.97，耗时最多扣 30%", 540, 18, COLOR_HINT);
}







static void drawCustomResult(Font font, const CustomGameResult *r, int saveResult)
{
    drawText(font, "挑战完成", 55, 34, COLOR_TITLE);
    drawText(font, TextFormat("正确 %d 组 +%d   错误 %d 次 -%d", r->correct, r->correct*100, r->wrong, r->wrong*40), 120, 22, COLOR_TITLE);
    drawText(font, TextFormat("基础积分 %d", r->baseScore), 160, 24, COLOR_TITLE);
    drawText(font, TextFormat("棋盘倍率 %.2f   开局倍率 %.2f", r->boardMultiplier, r->previewMultiplier), 205, 22, COLOR_HINT);
    drawText(font, TextFormat("加分倍率 %.2f   扣分倍率 %.2f", r->plusMultiplier, r->minusMultiplier), 240, 22, COLOR_HINT);
    drawText(font, TextFormat("难度修正后 %.2f 分", 100*r->correct*r->plusMultiplier-40*r->wrong*r->minusMultiplier), 275, 22, COLOR_TITLE);
    drawText(font, TextFormat("主动亮牌 %d 次   剩余 %.2f%%", r->reveals, 100*r->revealFactor), 310, 22, COLOR_HINT);
    drawText(font, TextFormat("用时 %.2f 秒 / 基准 %d 秒   扣 %.2f%%", r->elapsedSeconds, 5*r->groups, 100*r->timePenalty), 345, 22, COLOR_HINT);
    drawText(font, TextFormat("最终积分 %d 分", r->finalScore), 390, 32, COLOR_OK);
    drawText(font, saveResult == 1 ? "积分已保存" : saveResult == 0 ?
             "未刷新成绩或未进入前五" : "积分保存失败", 435, 20,
             saveResult == 1 ? COLOR_OK : saveResult == 0 ? COLOR_HINT : COLOR_ERROR);
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
    drawText(font, "行列可选 2 到 6，总牌数须为偶数；开局展示 1 到 30 秒。", 370, 20, COLOR_TITLE);
    drawText(font, "固定两张配对，正确一组加 100 基础分，错误一次扣 40 分。", 405, 20, COLOR_TITLE);
    drawText(font, "通关按棋盘及展示时间调整积分；每次主动亮牌再乘 0.97。", 440, 20, COLOR_TITLE);
    drawText(font, "耗时以每组 5 秒为基准，最多扣 30%；积分榜保留个人最高分。", 475, 19, COLOR_HINT);
    drawButton(introduceBackButton, "返回", font);
}
